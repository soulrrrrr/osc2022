#include "sched.h"
#include "uart.h"
#include "utils.h"

extern Thread *current_thread;
static int i = 0;
void timer_interrupt() {
    unsigned long cntfrq_el0;
    asm volatile ("mrs %0, cntfrq_el0":"=r" (cntfrq_el0));
    asm volatile ("lsr %0, %0, #5":"=r" (cntfrq_el0) :"r"(cntfrq_el0));
    asm volatile ("msr cntp_tval_el0, %0" : : "r"(cntfrq_el0));
    timer_tick();
}