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

#define SYS_OPEN            11
#define SYS_CLOSE           12
#define SYS_WRITE           13
#define SYS_READ            14
#define SYS_MKDIR           15
#define SYS_MOUNT           16
#define SYS_CHDIR           17

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

// syscall number : 11
extern int open(const char *pathname, int flags);

// syscall number : 12
extern int close(int fd);

// syscall number : 13
// remember to return read size or error code
extern long write(int fd, const void *buf, unsigned long count);

// syscall number : 14
// remember to return read size or error code
extern long read(int fd, void *buf, unsigned long count);

// syscall number : 15
// you can ignore mode, since there is no access control
extern int mkdir(const char *pathname, unsigned mode);

// syscall number : 16
// you can ignore arguments other than target (where to mount) and filesystem (fs name)
extern int mount(const char *src, const char *target, const char *filesystem, unsigned long flags, const void *data);

// syscall number : 17
extern int chdir(const char *path);

#endif