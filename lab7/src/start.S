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

.section ".text.boot"

.global _start

_start:
    bl from_el2_to_el1
    bl set_exception_vector_table
    // set top of stack just before our code (stack grows to a lower address per AAPCS64)
    adr     x1, _start
    mov     sp, x1

    // clear bss
    adr     x1, __bss_start
    ldr     w2, =__bss_size
1:  cbz     w2, 2f
    str     xzr, [x1], #8
    sub     w2, w2, #1
    cbnz    w2, 1b

2:  // jump to C code, should not return
    bl      main
    // for failsafe, halt this core too
3:  wfe
    b       3b

from_el2_to_el1:
    mov x0, (1 << 31) // EL1 uses aarch64
    msr hcr_el2, x0
    mov x0, 0x5 // EL1h (SPSel = 1) with interrupt enabled
    msr spsr_el2, x0
    msr elr_el2, lr
    eret // return to EL1

.globl from_el1_to_el0
from_el1_to_el0:
    mov x0, 0
    msr spsr_el1, x0
    msr elr_el1, lr
    mov x0, 0x60000 // user space stack
    msr sp_el0, x0
    eret // return to EL0

// save general registers to stack
.macro save_all
    sub sp, sp, 32 * 9
    stp x0, x1, [sp ,16 * 0]
    stp x2, x3, [sp ,16 * 1]
    stp x4, x5, [sp ,16 * 2]
    stp x6, x7, [sp ,16 * 3]
    stp x8, x9, [sp ,16 * 4]
    stp x10, x11, [sp ,16 * 5]
    stp x12, x13, [sp ,16 * 6]
    stp x14, x15, [sp ,16 * 7]
    stp x16, x17, [sp ,16 * 8]
    stp x18, x19, [sp ,16 * 9]
    stp x20, x21, [sp ,16 * 10]
    stp x22, x23, [sp ,16 * 11]
    stp x24, x25, [sp ,16 * 12]
    stp x26, x27, [sp ,16 * 13]
    stp x28, x29, [sp ,16 * 14]

    mrs x21, sp_el0
    mrs	x22, elr_el1
	mrs	x23, spsr_el1

	stp	x30, x21, [sp, #16 * 15] 
	stp	x22, x23, [sp, #16 * 16]
.endm

// load general registers from stack
.macro load_all
    ldp	x22, x23, [sp, #16 * 16]
	ldp	x30, x21, [sp, #16 * 15] 

    msr sp_el0, x21
	msr	elr_el1, x22			
	msr	spsr_el1, x23

    ldp x0, x1, [sp ,16 * 0]
    ldp x2, x3, [sp ,16 * 1]
    ldp x4, x5, [sp ,16 * 2]
    ldp x6, x7, [sp ,16 * 3]
    ldp x8, x9, [sp ,16 * 4]
    ldp x10, x11, [sp ,16 * 5]
    ldp x12, x13, [sp ,16 * 6]
    ldp x14, x15, [sp ,16 * 7]
    ldp x16, x17, [sp ,16 * 8]
    ldp x18, x19, [sp ,16 * 9]
    ldp x20, x21, [sp ,16 * 10]
    ldp x22, x23, [sp ,16 * 11]
    ldp x24, x25, [sp ,16 * 12]
    ldp x26, x27, [sp ,16 * 13]
    ldp x28, x29, [sp ,16 * 14]
    add sp, sp, 32 * 9
.endm

exception_handler_loop:
    b exception_handler_loop

exception_handler:
    save_all
    bl exception_handler_loop
    load_all
    eret

sync_exception_handler:
    save_all
    mrs x0, esr_el1 // to decide is syscall or not
    mrs x1, elr_el1 // the address return to 
    mov x2, sp      // trapframe
    bl sync_exc_router
    load_all
    eret

irq_exception_handler_low:
    save_all
    mov x0, #0
    bl timer_interrupt
    load_all
    eret

irq_exception_handler:
    save_all
    mov x0, #1
    bl timer_interrupt
    load_all
    eret

exception_handler_lower_irq:
    save_all
    bl timer_interrupt
    load_all
    eret

.align 11 // vector table should be aligned to 0x800
.global exception_vector_table
exception_vector_table:
    b exception_handler // branch to a handler function.
    .align 7 // entry size is 0x80, .align will pad 0
    b exception_handler
    .align 7
    b exception_handler
    .align 7
    b exception_handler
    .align 7

    b sync_exception_handler
    .align 7
    b irq_exception_handler
    .align 7
    b exception_handler
    .align 7
    b exception_handler
    .align 7

    b sync_exception_handler
    .align 7
    b irq_exception_handler_low
    .align 7
    b exception_handler
    .align 7
    b exception_handler
    .align 7

    b exception_handler
    .align 7
    b exception_handler
    .align 7
    b exception_handler
    .align 7
    b exception_handler
    .align 7

set_exception_vector_table:
    adr x0, exception_vector_table
    msr vbar_el1, x0
    ret

.equ CORE0_TIMER_IRQ_CTRL, 0x40000040

.global core_timer_enable
core_timer_enable:
    mov x0, 1
    msr cntp_ctl_el0, x0 // enable
    mrs x0, cntfrq_el0
    lsr x0, x0, #5
    msr cntp_tval_el0, x0 // set expired time
    mov x0, 2
    ldr x1, =CORE0_TIMER_IRQ_CTRL
    str w0, [x1] // unmask timer interrupt
    ret

.global switch_to
switch_to:
    stp x19, x20, [x0, 16 * 0]
    stp x21, x22, [x0, 16 * 1]
    stp x23, x24, [x0, 16 * 2]
    stp x25, x26, [x0, 16 * 3]
    stp x27, x28, [x0, 16 * 4]
    stp fp, lr, [x0, 16 * 5]
    mov x9, sp
    str x9, [x0, 16 * 6]

    ldp x19, x20, [x1, 16 * 0]
    ldp x21, x22, [x1, 16 * 1]
    ldp x23, x24, [x1, 16 * 2]
    ldp x25, x26, [x1, 16 * 3]
    ldp x27, x28, [x1, 16 * 4]
    ldp fp, lr, [x1, 16 * 5]
    ldr x9, [x1, 16 * 6]
    mov sp,  x9
    msr tpidr_el1, x1
    ret

.global get_current
get_current:
    mrs x0, tpidr_el1
    ret

.globl delay
delay:
	subs x0, x0, #1
	bne delay
	ret

.globl enable_irq
enable_irq:
	msr DAIFClr, #2 // IRQ mask bit
	ret

.globl disable_irq
disable_irq:
	msr	DAIFSet, #2
	ret

.globl run_thread
run_thread:
    bl      preempt_enable
    blr     x19         //should never return
    bl      end_thread

.globl ret_from_fork
ret_from_fork:
    load_all
    eret
    
.global memzero
memzero:
    str     xzr, [x0], #8
    sub     w1, w1, #1
    cbnz    w1, memzero
    ret
