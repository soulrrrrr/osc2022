#include "uart.h"
#include "utils.h"

void main() {
    uart_init();

    uart_puts("Hello from Raspberry pi!\n");
    char input[1024];
    unsigned long el;

    asm volatile ("mrs %0, CurrentEL" : "=r" (el));

    uart_puts("Current EL is: ");
    uart_hex((el>>2)&3); // current el saved in bits[3:2]
    uart_puts("\n");

    while (1) {
        uart_send('\r');
        uart_puts("# ");
        shell_input(input);
        if (strcmp(input, "run") == 0) {
            uart_puts("running...\n");
            void *program_address = get_user_program_address();
            asm volatile ("mov x0, 0"); // 被 0x3c0 juke了, 3c0 是關 interrupt
            asm volatile ("msr spsr_el1, x0"); //Holds the saved process state when an exception is taken to EL1
            asm volatile ("msr elr_el1, %0": :"r" (program_address));
            asm volatile ("mov x0, 0x60000");
            asm volatile ("msr sp_el0, x0");
            asm volatile ("eret");

        } else {
            uart_puts("Error input!\n");
        }
    }
}
