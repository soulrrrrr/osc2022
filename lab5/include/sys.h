#ifndef	_SYS_H
#define	_SYS_H

#define SYS_GETPID          0
#define SYS_UART_READ       1
#define SYS_UART_WRITE      2
#define SYS_EXEC            3
#define SYS_FORK            4
#define SYS_EXIT            5
#define SYS_MBOX_CALL       6
#define SYS_KILL            7

#endif

#ifndef __ASSEMBLY__

#include "typedef.h"

/* Function in sys.S */
extern int getpid();
extern size_t uartread(char buf[], size_t size);
extern size_t uartwrite(const char buf[], size_t size);
extern int exec(const char *name, char *const argv[]);
extern int fork();
extern void exit(int status);
extern int mbox_call(unsigned char ch, unsigned int *mbox);
extern void kill(int pid);

#endif