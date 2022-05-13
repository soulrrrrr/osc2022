#define __ASSEMBLY__
#include "sys.h"

.global getpid
getpid:
    mov x8, SYS_GETPID
    svc #0
    ret

.global uartread
uartread:
    mov x8, SYS_UART_READ
    svc #0
    ret

.global uartwrite
uartwrite:
    mov x8, SYS_UART_WRITE
    svc #0
    ret

.global exec
exec:
    mov x8, SYS_EXEC
    svc #0
    ret // shouldn't go here

.global fork
fork:
    mov x8, SYS_FORK
    svc #0
    ret

.global exit
exit:
    mov x8, SYS_EXIT
    svc #0
    ret

.global mbox_call
mbox_call:
    mov x8, SYS_MBOX_CALL
    svc #0
    ret

.global kill
kill:
    mov x8, SYS_KILL
    svc #0
    ret