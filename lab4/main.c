#include "uart.h"
#include "utils.h"
#include "freelist.h"
#include "memory.h"
#include "printf.h"


void main() {
    init_printf(NULL, putc);
    uart_init();
    memory_init();
    uart_puts("Hello from Raspberry pi!\n");
    char input[1024];
    while (1) {
        uart_send('\r');
        uart_puts("# ");
        shell_input(input);
        if (strcmp(input, "run") == 0) {
            uart_puts("running...\n");
        } else if (strcmp(input, "m") == 0) {
            shell_input(input);
            int size = (int)cstr_to_ulong(input);
            void *ptr = malloc(size);
            uart_puts("Allocation finished: ");
            uart_hex((uint)ptr);
            uart_puts("\n");
        } else if (strcmp(input, "d") == 0) {
            shell_input(input);
            void *ptr = (void *)(ulong)hex_to_int(input, 8);
            free(ptr);
            uart_puts("Free finished: ");
            uart_hex((uint)ptr);
            uart_puts("\n");
        } else if (strcmp(input, "pm") == 0) {
            print_freelists();
            print_memory();
        } else {
            uart_puts("Error input!\n");
        }
    }
}
