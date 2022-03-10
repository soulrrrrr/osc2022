#include "mbox.h"
#include "reboot.h"
#include "uart.h"

int strcmp(char *s1, char *s2) {
    while (*s1 != '\0' && *s1 == *s2) {
        s1++;
        s2++;
    }
    return (*(unsigned char *)s1) - (*(unsigned char *)s2);
}

void main() {
    uart_init();
    uart_puts("Hello from Raspberry pi!\n");

    while (1) {
        uart_send('\r');
        uart_send('#');
        uart_send(' ');
        int i = 0;
        char input[1024];
        char temp;
        while (1) {
            temp = uart_getc();
            if (temp == '\n') {
                uart_puts("\n");
                input[i] = '\0';
                break;
            } else
                uart_send(temp);

            input[i] = temp;
            i++;
        }
        if (strcmp(input, "help") == 0) {
            uart_puts("help\t: print this help memu\n");
            uart_puts("hello\t: print Hello World!\n");
            uart_puts("mailbox\t: print mailbox information\n");
            uart_puts("reboot\t: reboot the device\n");
        } else if (strcmp(input, "hello") == 0)
            uart_puts("Hello World!\n");
        else if (strcmp(input, "mailbox") == 0) {
            if (get_board_revision()) {
                uart_puts("My board revision is : ");
                uart_hex(mbox[5]);
                uart_puts("\n");
            } else
                uart_puts("Failed to get board revision.\n");

            if (get_arm_memory()) {
                uart_puts("My ARM memory base address is : ");
                uart_hex(mbox[5]);
                uart_puts("\n");
                uart_puts("My ARM memory size is : ");
                uart_hex(mbox[6]);
                uart_puts("\n");
            } else
                uart_puts("Failed to get arm memory.\n");
        } else if (strcmp(input, "reboot") == 0) {
            uart_puts("\n");
            reset(10);
        } else
            uart_puts("Error input!\n");
    }
}
