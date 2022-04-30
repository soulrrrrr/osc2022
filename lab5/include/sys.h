#ifndef	_SYS_H
#define	_SYS_H

#define SYS_GETPID     0
#define SYS_UART_READ       1
#define SYS_UART_WRITE      2
#define SYS_EXEC            3
#define SYS_FORK            4
#define SYS_EXIT            5

#endif

#ifndef __ASSEMBLY__

#include "typedef.h"

/* Function in sys.S */
extern int get_pid();
extern size_t uart_read(char buf[], size_t size);
extern size_t uart_write(const char buf[], size_t size);
extern int exec(void(*func)());
extern int fork();
extern void exit();


#endif