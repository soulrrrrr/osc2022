// https://developer.arm.com/documentation/ddi0595/2021-06/AArch64-Registers/ESR-EL1--Exception-Syndrome-Register--EL1-

#include "utils.h"
#include "exception.h"
#include "sys.h"
#include "printf.h"
#include "sched.h"
#include "memory.h"
#include "uart.h"
#include "mbox.h"
#include "mmu.h"

extern void end_thread(void);
extern void ret_from_fork(void);
extern Thread *task[];
extern void delay(int);
extern void update_pgd(uint64_t);

void sync_exc_router(uint64_t esr_el1, uint64_t elr_el1, Trapframe *trapframe) {
    int ec = (esr_el1 >> 26) & 0b111111;
    int iss = esr_el1 & 0x1FFFFFF;
    if (ec == 0b010101) {  // is system call
        uint64_t syscall_num = trapframe->x[8];
        //printf("[SYSCALL] %d\n", syscall_num);
        syscall(syscall_num, trapframe);
    }
    else {
        printf("Exception return address 0x%x\n", elr_el1);
        printf("Exception class (EC) 0x%x\n", ec);
        printf("Instruction specific syndrome (ISS) 0x%x\n", iss);
        while(1){
            ;
        }
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

        case SYS_EXEC:
            sys_exec(trapframe);
            break;

        case SYS_FORK:
            sys_fork(trapframe);
            break;

        case SYS_EXIT:
            sys_exit(trapframe);
            break;

        case SYS_MBOX_CALL:
            sys_mbox_call(trapframe);
            break;
        case SYS_KILL:
            sys_kill(trapframe);
            break;
    }
    return;
}

void sys_getpid(Trapframe *trapframe) {
    trapframe->x[0] = current_thread()->pid;
}

void sys_uart_read(Trapframe *trapframe) {
    char *buf = (char *)trapframe->x[0];
    size_t size = (size_t)trapframe->x[1];
    enable_irq(); // 避免卡在 read 裡
    for (int i = 0; i < size; i++) {
        *(buf + i) = uart_getc();
    }
    disable_irq();
    trapframe->x[0] = size;
}

void sys_uart_write(Trapframe *trapframe) {
    const char *buf = (const char *)trapframe->x[0];
    size_t size = (size_t)trapframe->x[1];
    enable_irq();
    for (int i = 0; i < size; i++) {
        uart_send(*(buf + i));
    }
    disable_irq();
    trapframe->x[0] = size;
}

void sys_exec(Trapframe *trapframe) {
    preempt_disable();
    char *input = (char *)trapframe->x[0];
    char *program_pos;
    cpio_newc_header *fs = (cpio_newc_header *)0xffff000008000000;
    char *current = (char *)fs;
    int name_size;
    int file_size;
    while (1) {
        fs = (cpio_newc_header *)current;
        name_size = hex_to_int(fs->c_namesize, 8);
        file_size = hex_to_int(fs->c_filesize, 8);
        current += 110;
        if (strcmp(current, "TRAILER!!!") == 0) {
            uart_puts("No such file!\n");
            break;
        }
        if (strcmp(current, input) == 0) {
            current += name_size;
            while ((current - (char *)fs) % 4 != 0)
                current++;
            program_pos = (char *)current;
            break;
        } else {
            current += name_size;
            while ((current - (char *)fs) % 4 != 0)
                current++;
            current += file_size;
            while ((current - (char *)fs) % 4 != 0)
                current++;
        }
    }
    printf("program pos: %x\n", program_pos);
    printf("file size: %d\n", file_size);
    uint64_t new_program_va = (uint64_t)malloc(file_size);
    uint64_t new_program_pa = vir_to_phy(new_program_va);
    uint64_t user_stack_va = (uint64_t)malloc(USER_STACK_SIZE);
    uint64_t user_stack_pa = vir_to_phy(user_stack_va);
    mappages((pagetable *)current_thread()->pgd, USER_PC, file_size, new_program_pa); // map user program's code
    mappages((pagetable *)current_thread()->pgd, USER_STACK_LOW, USER_STACK_SIZE, user_stack_pa); // map user program's stack
    mappages((pagetable *)current_thread()->pgd, 0x3c000000, 0x3000000, 0x3c000000); // map user program's mbox
    for (int i = 0; i < file_size; i++) {
        *((char *)new_program_va+i) = *(program_pos+i);
    }
    asm volatile("msr sp_el0, %0" : : "r"(USER_SP));
    asm volatile("msr elr_el1, %0": : "r"(USER_PC));
    asm volatile("msr spsr_el1, %0" : : "r"(0x0));
    update_pgd(current_thread()->pgd);
    preempt_enable();
    asm volatile("eret");
    trapframe->x[0] = 0;
}

void sys_fork(Trapframe *trapframe) {
    preempt_disable();
    Thread *parent = current_thread();
    /* 
        ret_from_fork 會把 child_trapframe load 到 register，
        這樣跑 child thread 時就會用到 child_trapframe 更改的 sp
    */
    int newpid = thread_create(ret_from_fork);

    Thread *child = task[newpid];
    
    printf("child pid: %x\n", newpid);
    
    // copy kernel stack (including trapframe)
    uint64_t kstack_offset = (char *)parent->kernel_sp - (char *)trapframe;
    for (uint64_t i = 1; i <= kstack_offset; i++) {
        *((char *)(child->kernel_sp - i)) = *((char *)(parent->kernel_sp - i));
    }
    child->cpu_context.sp = child->kernel_sp - kstack_offset;

    // copy all user program & stack
    printf("Parent PGD: %x, User PGD: %x\n", parent->pgd, child->pgd);
    if (parent->pgd != 0) copypages(phy_to_vir(parent->pgd), phy_to_vir(child->pgd), 0);

    Trapframe *child_trapframe = (Trapframe *)child->cpu_context.sp;
    child_trapframe->sp_el0 = trapframe->sp_el0;

    trapframe->x[0] = child->pid;
    child_trapframe->x[0] = 0;
    preempt_enable();
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

void sys_kill(Trapframe *trapframe) {
    int pid = trapframe->x[0];
    task[pid]->state = TASK_ZOMBIE;
}

void timer_interrupt(int i) {
    unsigned long cntfrq_el0;
    asm volatile ("mrs %0, cntfrq_el0":"=r" (cntfrq_el0));
    asm volatile ("lsr %0, %0, #5":"=r" (cntfrq_el0) :"r"(cntfrq_el0)); // 1/32 second tick
    asm volatile ("msr cntp_tval_el0, %0" : : "r"(cntfrq_el0));
    timer_tick();
}
