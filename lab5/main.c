#include "uart.h"
#include "utils.h"
#include "freelist.h"
#include "memory.h"
#include "sched.h"
#define N 5

extern void delay();
extern Thread *current_thread;

void foo(){
    for(int i = 0; i < 10; ++i) {
        debug("pid", current_thread->pid);
        debug("i", i);
        //printf("Thread id: %d %d\n", current_thread.id(), i);
        delay(1000000);
        schedule();
    }
}

void main() {
    uart_init();
    memory_init();
    while (1) {
        char c = uart_getc();
        uart_send(c);
        if (c == 's') break;
    }
	enable_irq();
    uart_puts("\nHello from Raspberry pi!\n");

    unsigned long el;

    asm volatile ("mrs %0, CurrentEL" : "=r" (el));

    uart_puts("Current EL is: ");
    uart_hex((el>>2)&3); // current el saved in bits[3:2]
    uart_puts("\n");
    for(int i = 0; i < N; ++i) { // N should > 2
        thread_create(foo);
    }
    idle();
    // char input[1024];
    // while (1) {
    //     uart_send('\r');
    //     uart_puts("# ");
    //     shell_input(input);
    //     if (strcmp(input, "run") == 0) {
    //         uart_puts("running...\n");
    //     } else if (strcmp(input, "pm") == 0) {
    //         print_freelists();
    //         print_memory();
    //     } else {
    //         uart_puts("Error input!\n");
    //     }
    // }
}
