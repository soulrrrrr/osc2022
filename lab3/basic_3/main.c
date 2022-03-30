#include "uart.h"
#include "utils.h"
#include "queue.h"

void main() {
    uart_init();
    from_el1_to_el0();

    uart_puts("Hello from Raspberry pi!\n");
    char input[1024];
    while (1) {
        uart_send('\r');
        uart_puts("# ");
        shell_input(input);
        if (strcmp(input, "run") == 0) {
            uart_puts("running...\n");
        } else {
            uart_puts("Error input!\n");
        }
    }
}
