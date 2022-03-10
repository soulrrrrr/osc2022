/*
 * Copyright (C) 2018 bzt (bztsrc@github)
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#include "mbox.h"
#include "reboot.h"
#include "uart.h"

/**
 * @brief compare string
 *
 * @param a first string
 * @param b second string
 */
int strcmp(char *a, char *b) {
    while (*a != '\0' && *a == *b) {
        a++;
        b++;
    }
    return (*(unsigned char *)a) - (*(unsigned char *)b);
}

void main() {
    // set up serial console
    uart_init();
    uart_puts("Hello from Raspberry pi!\n");

    char input[1024];
    int len;

    while (1) {
        len = 0;
        uart_puts("\r# ");
        while (1) {
            input[len] = uart_getc();
            if (input[len] == '\n') {
                input[len] = '\0';
                uart_puts("\n");
                break;
            }
            uart_send(input[len++]);
            if (len >= 1023) break; // char array size
        }
        if (strcmp(input, "help") == 0) {
            uart_puts("help\t: print this help menu\n");
            uart_puts("hello\t: print Hello World!\n");
            uart_puts("mailbox\t: show board revision and ARM memory size\n");
            uart_puts("reboot\t: reboot the device\n");
        } else if (strcmp(input, "hello") == 0) {
            uart_puts("Hello World!\n");
        } else if (strcmp(input, "mailbox") == 0) {
            if (!get_board_revision()) {
                uart_puts("Fail to get board revision!\n");
                continue;
            }
            uart_puts("Board revision: ");
            uart_hex(mbox[5]);
            uart_puts("\n");
            if (!get_arm_memory()) {
                uart_puts("Fail to get ARM memory!\n");
                continue;
            }
            uart_puts("Base address: 0x");
            uart_hex(mbox[5]);
            uart_puts("\n");
            uart_puts("ARM memory size: ");
            uart_uint(mbox[6]);
            uart_puts(" bytes\n");
        } else if (strcmp(input, "reboot") == 0) {
            uart_puts("rebooting...\n");
            reset(10); // tick = 10
        }
    }
}
