// https://developer.arm.com/documentation/ddi0595/2021-06/AArch64-Registers/ESR-EL1--Exception-Syndrome-Register--EL1-

#include "utils.h"
#include "exception.h"
#include "sys.h"
#include "printf.h"
#include "sched.h"

void sync_exc_router(uint64_t esr_el1, uint64_t elr_el1, Trapframe *trapframe) {
    int ec = (esr_el1 >> 26) & 0b111111;
    int iss = esr_el1 & 0x1FFFFFF;
    if (ec == 0b010101) {  // is system call
        uint64_t syscall_num = trapframe->x[8];
        syscall(syscall_num, trapframe);
    }
    else {
        //return;
        printf("Exception return address 0x%x\n", elr_el1);
        printf("Exception class (EC) 0x%x\n", ec);
        printf("Instruction specific syndrome (ISS) 0x%x\n", iss);
    }
}

void syscall(uint64_t syscall_num, Trapframe* trapframe) {
    return;
    // switch (syscall_num) {
    //     case SYS_GETPID:
    //         sys_getpid(trapframe);
    //         break;

    //     case SYS_UART_READ:
    //         sys_uart_read(trapframe);
    //         break;

    //     case SYS_UART_WRITE:
    //         sys_uart_write(trapframe);
    //         break;

    //     case SYS_EXEC:
    //         sys_exec(trapframe);
    //         break;

    //     case SYS_FORK:
    //         sys_fork(trapframe);
    //         break;

    //     case SYS_EXIT:
    //         sys_exit(trapframe);
    //         break;
    // }
}

void timer_interrupt() {
    unsigned long cntfrq_el0;
    asm volatile ("mrs %0, cntfrq_el0":"=r" (cntfrq_el0));
    asm volatile ("lsr %0, %0, #5":"=r" (cntfrq_el0) :"r"(cntfrq_el0)); // 1/32 second tick
    asm volatile ("msr cntp_tval_el0, %0" : : "r"(cntfrq_el0));
    timer_tick();
}