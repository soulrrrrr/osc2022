#ifndef _EXCEPTION_H
#define _EXCEPTION_H

#include "typedef.h"

// save_all, load_all 的 register 用 struct 包起來，方便操作
typedef struct Trapframe {
    uint64_t x[31]; // general register from x0 ~ x30
    uint64_t sp_el0;
    uint64_t elr_el1;
    uint64_t spsr_el1;
} Trapframe;

typedef struct cpio_newc_header {
    char c_magic[6];
    char c_ino[8];
    char c_mode[8];
    char c_uid[8];
    char c_gid[8];
    char c_nlink[8];
    char c_mtime[8];
    char c_filesize[8];
    char c_devmajor[8];
    char c_devminor[8];
    char c_rdevmajor[8];
    char c_rdevminor[8];
    char c_namesize[8];
    char c_check[8];
} cpio_newc_header;

extern void enable_irq();
extern void disable_irq();
void sync_exception_router(uint64_t esr_el1, uint64_t elr_el1, Trapframe *trapframe);
void syscall(uint64_t syscall_num, Trapframe* trapframe);
void sys_getpid(Trapframe *trapframe);
void sys_uart_read(Trapframe *trapframe);
void sys_uart_write(Trapframe *trapframe);
void sys_exec(Trapframe *trapframe);
void sys_fork(Trapframe *trapframe);
void sys_exit(Trapframe *trapframe);
void sys_mbox_call(Trapframe *trapframe);
void sys_kill(Trapframe *trapframe);
void sys_open(Trapframe *trapframe);
void sys_close(Trapframe *trapframe);
void sys_write(Trapframe *trapframe);
void sys_read(Trapframe *trapframe);
void sys_mkdir(Trapframe *trapframe);
void sys_mount(Trapframe *trapframe);
void sys_chdir(Trapframe *trapframe);


#endif /* _EXCEPTION_H */