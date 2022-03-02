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

#include "uart.h"
#include "mbox.h"
#include "reboot.h"

/**
 * @brief compare string
 * 
 * @param a first string 
 * @param b second string 
 * @param n compart bytes
 * @return int 1 means a equals b, 0 means a doesn't equals b
 */
int strncmp(char *a, char *b, int n) {
   for (int i = 0; i < n; i++) {
       if (*(a+i) != *(b+i)) return 0;
   }
   return 1;
}

void main()
{
    // set up serial console
    uart_init();
    uart_puts("Hello from Raspberry pi!\n");
    
    char input[1024];
    int len;

    while(1) {
        len = 0;
        uart_send('\r');
        uart_send('#');
        uart_send(' ');
        while(1) {
            input[len] = uart_getc();
            if (input[len] == '\n') {
                uart_puts("\n");
                break;
            }
            uart_send(input[len++]);
            if (len >= 1024) break; // char array size
        }
        if (strncmp(input, "help", 4)) {
            uart_puts("help\t: print this help menu\n");
            uart_puts("hello\t: print Hello World!\n");
            uart_puts("mailbox\t: show board revision and ARM memory size\n");
            uart_puts("reboot\t: reboot the device\n");
        }
        else if (strncmp(input, "hello", 5)) {
            uart_puts("Hello World!\n");
        }
        else if (strncmp(input, "mailbox", 7)) {
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
        }
        else if (strncmp(input, "reboot", 6)) {
            uart_puts("rebooting...\n");
            reset(10); // tick = 10
        }
    }
}
