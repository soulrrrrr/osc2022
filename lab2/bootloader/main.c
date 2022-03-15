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

void main() {
    // set up serial console
    uart_init();
    char c;
    while((c = uart_getc()) != '\n')
        ;
    volatile unsigned char *bootloader = (unsigned char *)0x60000;
    volatile unsigned char *code = (unsigned char *)0x80000;
    for(int i = 0; i < 0x2000; i++) {
        *(bootloader + i) = *(code + i);
    }

    asm volatile("b #-0x1FFFC");
    
    uart_puts("Loading...\n");

    // int image_size = 0;
    // char c;
    // while((c = uart_getc()) != '\n') {
    //     uart_send(c);
    //     image_size *= 10;
    //     image_size += (c - '0');
    // }

    // uart_puts("\nImage size: ");
    // uart_uint((unsigned int)image_size);
    // uart_puts(" bytes\n");

    for (int i = 0; i < 2606; i++) {
        *(code + i) = (unsigned char)uart_getc_pure();
    }
    uart_puts("Finish loading image\n");

    asm volatile("mov x19, #0x80000");
    asm volatile("add x19, x19, #0x14");
    asm volatile("br x19");

}
