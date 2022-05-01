// https://developer.arm.com/documentation/ddi0595/2021-06/AArch64-Registers/ESR-EL1--Exception-Syndrome-Register--EL1-

#include "utils.h"
#include "exception.h"
#include "sys.h"
#include "printf.h"
#include "sched.h"
#include "memory.h"
#include "uart.h"
#include "mbox.h"

extern void end_thread(void);
extern void ret_from_fork(void);
extern Thread *task[];

void sync_exc_router(uint64_t esr_el1, uint64_t elr_el1, Trapframe *trapframe) {
    int ec = (esr_el1 >> 26) & 0b111111;
    int iss = esr_el1 & 0x1FFFFFF;
    if (ec == 0b010101) {  // is system call
        uint64_t syscall_num = trapframe->x[8];
        printf("[SYSCALL] %d\n", syscall_num);
        syscall(syscall_num, trapframe);
    }
    else {
        return;
        printf("Exception return address 0x%x\n", elr_el1);
        printf("Exception class (EC) 0x%x\n", ec);
        printf("Instruction specific syndrome (ISS) 0x%x\n", iss);
    }
}

void syscall(uint64_t syscall_num, Trapframe* trapframe) {
    switch (syscall_num) {
        case SYS_GETPID:
            sys_getpid(trapframe);
            break;

        case SYS_UART_READ:
            sys_uart_read(trapframe);
            break;

        case SYS_UART_WRITE:
            sys_uart_write(trapframe);
            break;

        // case SYS_EXEC:
        //     sys_exec(trapframe);
        //     break;

        case SYS_FORK:
            sys_fork(trapframe);
            break;

        case SYS_EXIT:
            sys_exit(trapframe);
            break;

        case SYS_MBOX_CALL:
            sys_mbox_call(trapframe);
            break;
    }
    return;
}

void timer_interrupt(int i) {
    unsigned long cntfrq_el0;
    asm volatile ("mrs %0, cntfrq_el0":"=r" (cntfrq_el0));
    asm volatile ("lsr %0, %0, #5":"=r" (cntfrq_el0) :"r"(cntfrq_el0)); // 1/32 second tick
    asm volatile ("msr cntp_tval_el0, %0" : : "r"(cntfrq_el0));
    timer_tick();
}

void sys_getpid(Trapframe *trapframe) {
    trapframe->x[0] = current_thread()->pid;
}

void sys_uart_read(Trapframe *trapframe) {
    char *buf = (char *)trapframe->x[0];
    size_t size = (size_t)trapframe->x[1];
    for (int i = 0; i < size; i++) {
        *(buf + i) = uart_getc();
    }
    trapframe->x[0] = size;
}

void sys_uart_write(Trapframe *trapframe) {
    const char *buf = (const char *)trapframe->x[0];
    size_t size = (size_t)trapframe->x[1];
    for (int i = 0; i < size; i++) {
        uart_send(*(buf + i));
    }
    trapframe->x[0] = size;
}

void sys_fork(Trapframe *trapframe) {
    Thread *parent = current_thread();
    /* 
        ret_from_fork 會把 child_trapframe load to register，
        這樣跑 child thread 時就會用到 child_trapframe 更改的 sp
    */
    int newpid = thread_create(ret_from_fork);

    Thread *child = task[newpid];
    
    printf("child: %x\n", child);
    
    // copy kernel stack and user stack
    uint64_t kstack_offset = (char *)parent->kernel_sp - (char *)trapframe;
    uint64_t ustack_offset = (char *)parent->user_sp - (char *)trapframe->sp_el0;

    // copy kernel stack (including trapframe)
    for (uint64_t i = 1; i <= kstack_offset; i++) {
        *((char *)(child->kernel_sp - i)) = *((char *)(parent->kernel_sp - i));
    }

    // copy user stack
    for (uint64_t i = 1; i <= ustack_offset; i++) {
        *((char *)(child->user_sp - i)) = *((char *)(parent->user_sp - i));
    }

    child->cpu_context.sp = child->kernel_sp - kstack_offset;

    Trapframe *child_trapframe = (Trapframe *)child->cpu_context.sp;
    child_trapframe->sp_el0 = child->user_sp - ustack_offset;
    child_trapframe->spsr_el1 = 0x0; // el0

    trapframe->x[0] = child->pid;
    child_trapframe->x[0] = 0;
}

void sys_exit(Trapframe *trapframe) {
    current_thread()->status = trapframe->x[0];
    end_thread();
}

void sys_mbox_call(Trapframe *trapframe) {
    unsigned char ch = (unsigned char)trapframe->x[0];
    unsigned int *mbox = (unsigned int *)trapframe->x[1];
    int ret = mboxc_mbox_call(ch, mbox); // defined in mbox.c
    trapframe->x[0] = ret;
}