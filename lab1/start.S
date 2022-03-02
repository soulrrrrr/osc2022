.section ".text.boot"

.global _start

_start:
// set stack pointer and branch to main function.
2:
    ldr x0, = _start
    mov sp, x0
    bl main
1:
    b 1b
