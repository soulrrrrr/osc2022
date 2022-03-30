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

#include "gpio.h"
#include "queue.h"

/* Auxilary mini UART registers */
#define AUX_ENABLE ((volatile unsigned int *)(MMIO_BASE + 0x00215004))
#define AUX_MU_IO ((volatile unsigned int *)(MMIO_BASE + 0x00215040))
#define AUX_MU_IER ((volatile unsigned int *)(MMIO_BASE + 0x00215044))
#define AUX_MU_IIR ((volatile unsigned int *)(MMIO_BASE + 0x00215048))
#define AUX_MU_LCR ((volatile unsigned int *)(MMIO_BASE + 0x0021504C))
#define AUX_MU_MCR ((volatile unsigned int *)(MMIO_BASE + 0x00215050))
#define AUX_MU_LSR ((volatile unsigned int *)(MMIO_BASE + 0x00215054))
#define AUX_MU_MSR ((volatile unsigned int *)(MMIO_BASE + 0x00215058))
#define AUX_MU_SCRATCH ((volatile unsigned int *)(MMIO_BASE + 0x0021505C))
#define AUX_MU_CNTL ((volatile unsigned int *)(MMIO_BASE + 0x00215060))
#define AUX_MU_STAT ((volatile unsigned int *)(MMIO_BASE + 0x00215064))
#define AUX_MU_BAUD ((volatile unsigned int *)(MMIO_BASE + 0x00215068))

#define IRQS1 ((volatile unsigned int *)(0x3f00b210))


struct queue uart_write, uart_read;
/**
 * Set baud rate and characteristics (115200 8N1) and map to GPIO
 */
void uart_init() {
    register unsigned int r;

    /* initialize UART */
    *AUX_ENABLE |= 1; // enable UART1, AUX mini uart
    *AUX_MU_IER = 1; // enable interrupt (2: I can transmit to uart, 1: I can receive from uart)
    *AUX_MU_CNTL = 0;
    *AUX_MU_LCR = 3; // 8 bits
    *AUX_MU_MCR = 0;
    *AUX_MU_IIR = 0xc6;
    *AUX_MU_BAUD = 270; // 115200 baud
    /* map UART1 to GPIO pins */
    r = *GPFSEL1;
    r &= ~((7 << 12) | (7 << 15)); // gpio14, gpio15
    r |= (2 << 12) | (2 << 15);    // alt5
    *GPFSEL1 = r;
    *GPPUD = 0; // enable pins 14 and 15
    r = 150;
    while (r--) {
        asm volatile("nop");
    }
    *GPPUDCLK0 = (1 << 14) | (1 << 15);
    r = 150;
    while (r--) {
        asm volatile("nop");
    }
    *GPPUDCLK0 = 0;   // flush GPIO setup
    *IRQS1 |= 1 << 29;
    *AUX_MU_CNTL = 3; // enable Tx, Rx

    queue_init(&uart_read, 1024);
    queue_init(&uart_write, 1024);
}

/**
 * Send a character
 */
void uart_send(unsigned int c) {
    int was_empty = 0;
    if (queue_empty(&uart_write)) {
        was_empty = 1;
    }
    queue_push(&uart_write, c); // push to write queue, drop if buffer full
    if (was_empty)
        *AUX_MU_IER |= 2; // enable interrupt (2: I can transmit to uart, 1: I can receive from uart)
    // do {
    //     asm volatile("nop");
    // } while (!(*AUX_MU_LSR & 0x20)); // transmitter idle
    // /* write the character to the buffer */
    // *AUX_MU_IO = c;
}

/**
 * Receive a character
 */
char uart_getc() {
    while (queue_empty(&uart_read)) {
        asm volatile ("nop");
    }
    char r = queue_pop(&uart_read);
    return r == '\r' ? '\n' : r;
    // char r;
    // /* wait until something is in the buffer */
    // do {
    //     asm volatile("nop");
    // } while (!(*AUX_MU_LSR & 0x01)); // receiver overrun
    // /* read it and return */
    // r = (char)(*AUX_MU_IO);
    // /* convert carrige return to newline */
    // return r == '\r' ? '\n' : r;
}

/**
 * Receive a character without converting CR to LF
 */
char uart_getc_pure() {
    char r;
    /* wait until something is in the buffer */
    do {
        asm volatile("nop");
    } while (!(*AUX_MU_LSR & 0x01)); // receiver overrun
    /* read it and return */
    r = (char)(*AUX_MU_IO);
    return r;
}
/**
 * Display an unsigned int
 */
void uart_uint(unsigned int i) {
    char c[10];
    if (i == 0) {
        uart_send('0');
        return;
    }
    int digits = -1;
    while (i != 0) {
        c[++digits] = '0' + i % 10;
        i /= 10;
    }
    for (; digits >= 0; --digits) {
        uart_send(c[digits]);
    }
}

/**
 * Display an unsigned long
 */
void uart_ulong(unsigned long i) {
    char c[20];
    if (i == 0) {
        uart_send('0');
        return;
    }
    int digits = -1;
    while (i != 0) {
        c[++digits] = '0' + i % 10;
        i /= 10;
    }
    for (; digits >= 0; --digits) {
        uart_send(c[digits]);
    }
}

/**
 * Display a string
 */
void uart_puts(char *s) {
    while (*s) {
        /* convert newline to carrige return + newline */
        if (*s == '\n')
            uart_send('\r');
        uart_send(*s++);
    }
}

/**
 * Display a binary value in hexadecimal
 */
void uart_hex(unsigned int d) {
    unsigned int n;
    int c;
    for (c = 28; c >= 0; c -= 4) {
        // get highest tetrad
        n = (d >> c) & 0xF;
        // 0-9 => '0'-'9', 10-15 => 'A'-'F'
        n += n > 9 ? 0x37 : 0x30;
        uart_send(n);
    }
}

void uart_hex_long(unsigned long d) {
    unsigned long n;
    int c;
    for (c = 60; c >= 0; c -= 4) {
        // get highest tetrad
        n = (d >> c) & 0xF;
        // 0-9 => '0'-'9', 10-15 => 'A'-'F'
        n += n > 9 ? 0x37 : 0x30;
        uart_send(n);
    }
}

void uart_handler() {
    if (*AUX_MU_IIR & 2) { // can transmit
        char c = queue_pop(&uart_write);
        *AUX_MU_IO = c;
        if (queue_empty(&uart_write)) {
            *AUX_MU_IER &= ~2; // disable transmit interrupt
        }
    }
    else if (*AUX_MU_IIR & 4) { // can receive
        if (queue_full(&uart_read)) return;
        char r = (char)(*AUX_MU_IO);
        queue_push(&uart_read, r);
    }
}