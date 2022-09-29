// https://developer.arm.com/documentation/ddi0595/2021-06/AArch64-Registers/ESR-EL1--Exception-Syndrome-Register--EL1-

#include "utils.h"
#include "exception.h"
#include "sys.h"
#include "printf.h"
#include "sched.h"
#include "memory.h"
#include "uart.h"
#include "mbox.h"
#include "vfs.h"
#include "tmpfs.h"

extern void end_thread(void);
extern void ret_from_fork(void);
extern Thread *task[];
extern void delay(int);

void sync_exc_router(uint64_t esr_el1, uint64_t elr_el1, Trapframe *trapframe) {
    int ec = (esr_el1 >> 26) & 0b111111;
    int iss = esr_el1 & 0x1FFFFFF;
    if (ec == 0b010101) {  // is system call
        uint64_t syscall_num = trapframe->x[8];
        //printf("[SYSCALL] %d\n", syscall_num);
        syscall(syscall_num, trapframe);
    }
    else {
        printf("Exception return address 0x%x\n", elr_el1);
        printf("Exception class (EC) 0x%x\n", ec);
        printf("Instruction specific syndrome (ISS) 0x%x\n", iss);
        while(1){
            ;
        }
    }
}

void syscall(uint64_t syscall_num, Trapframe* trapframe) {
    switch (syscall_num) {
        case SYS_GETPID:
            sys_getpid(trapframe);
            break;

        case SYS_UART_READ:
            sys_uart_read(trapframe);
            break;

        case SYS_UART_WRITE:
            sys_uart_write(trapframe);
            break;

        case SYS_EXEC:
            sys_exec(trapframe);
            break;

        case SYS_FORK:
            sys_fork(trapframe);
            break;

        case SYS_EXIT:
            sys_exit(trapframe);
            break;

        case SYS_MBOX_CALL:
            sys_mbox_call(trapframe);
            break;
        case SYS_KILL:
            sys_kill(trapframe);
            break;

        case SYS_OPEN:
            sys_open(trapframe);
            break;

        case SYS_CLOSE:
            sys_close(trapframe);
            break;

        case SYS_WRITE:
            sys_write(trapframe);
            break;

        case SYS_READ:
            sys_read(trapframe);
            break;

        case SYS_MKDIR:
            sys_mkdir(trapframe);
            break;

        case SYS_MOUNT:
            sys_mount(trapframe);
            break;

        case SYS_CHDIR:
            sys_chdir(trapframe);
            break;
    }
    return;
}

void sys_getpid(Trapframe *trapframe) {
    trapframe->x[0] = current_thread()->pid;
}

void sys_uart_read(Trapframe *trapframe) {
    char *buf = (char *)trapframe->x[0];
    size_t size = (size_t)trapframe->x[1];
    enable_irq(); // 避免卡在 read 裡
    for (int i = 0; i < size; i++) {
        *(buf + i) = uart_getc();
    }
    disable_irq();
    trapframe->x[0] = size;
}

void sys_uart_write(Trapframe *trapframe) {
    const char *buf = (const char *)trapframe->x[0];
    size_t size = (size_t)trapframe->x[1];
    enable_irq();
    for (int i = 0; i < size; i++) {
        uart_send(*(buf + i));
    }
    disable_irq();
    trapframe->x[0] = size;
}

void sys_exec(Trapframe *trapframe) {
    preempt_disable();
    char *input = (char *)trapframe->x[0];
    char *program_pos;
    cpio_newc_header *fs = (cpio_newc_header *)0x8000000;
    char *current = (char *)0x8000000;
    int name_size;
    int file_size;
    while (1) {
        fs = (cpio_newc_header *)current;
        name_size = hex_to_int(fs->c_namesize, 8);
        file_size = hex_to_int(fs->c_filesize, 8);
        current += 110;
        if (strcmp(current, "TRAILER!!!") == 0) {
            uart_puts("No such file!\n");
            break;
        }
        if (strcmp(current, input) == 0) {
            current += name_size;
            while ((current - (char *)fs) % 4 != 0)
                current++;
            program_pos = (char *)current;
            break;
        } else {
            current += name_size;
            while ((current - (char *)fs) % 4 != 0)
                current++;
            current += file_size;
            while ((current - (char *)fs) % 4 != 0)
                current++;
        }
    }
    char *new_program_pos = (char *)malloc(file_size);
    for (int i = 0; i < file_size; i++) {
        *(new_program_pos+i) = *(program_pos+i);
    }
    printf("program pos : %x\n", new_program_pos);
    Thread *cur = current_thread();
    preempt_enable();
    asm volatile("msr sp_el0, %0" : : "r"(cur->user_sp));
    asm volatile("msr elr_el1, %0": : "r"(new_program_pos));
    asm volatile("msr spsr_el1, %0" : : "r"(0x0));
    asm volatile("eret");
    trapframe->x[0] = 0;
}

void sys_fork(Trapframe *trapframe) {
    Thread *parent = current_thread();
    /* 
        ret_from_fork 會把 child_trapframe load to register，
        這樣跑 child thread 時就會用到 child_trapframe 更改的 sp
    */
    int newpid = thread_create(ret_from_fork);

    Thread *child = task[newpid];
    
    printf("child: %x\n", child);
    
    // copy kernel stack and user stack
    uint64_t kstack_offset = (char *)parent->kernel_sp - (char *)trapframe;
    uint64_t ustack_offset = (char *)parent->user_sp - (char *)trapframe->sp_el0;

    // copy kernel stack (including trapframe)
    for (uint64_t i = 1; i <= kstack_offset; i++) {
        *((char *)(child->kernel_sp - i)) = *((char *)(parent->kernel_sp - i));
    }

    // copy user stack
    for (uint64_t i = 1; i <= ustack_offset; i++) {
        *((char *)(child->user_sp - i)) = *((char *)(parent->user_sp - i));
    }

    child->cpu_context.sp = child->kernel_sp - kstack_offset;

    Trapframe *child_trapframe = (Trapframe *)child->cpu_context.sp;
    child_trapframe->sp_el0 = child->user_sp - ustack_offset;
    printf("child sp: %x\n", child_trapframe->sp_el0);

    trapframe->x[0] = child->pid;
    child_trapframe->x[0] = 0;
}

void sys_exit(Trapframe *trapframe) {
    current_thread()->status = trapframe->x[0];
    end_thread();
}

void sys_mbox_call(Trapframe *trapframe) {
    unsigned char ch = (unsigned char)trapframe->x[0];
    unsigned int *mbox = (unsigned int *)trapframe->x[1];
    int ret = mboxc_mbox_call(ch, mbox); // defined in mbox.c
    trapframe->x[0] = ret;
}

void sys_kill(Trapframe *trapframe) {
    int pid = trapframe->x[0];
    task[pid]->state = TASK_ZOMBIE;
}

void sys_open(Trapframe *trapframe) {
    printf("Open\n");
    char *path = trapframe->x[0];
    int flags = trapframe->x[1];
    struct file *handle;
    int ret = vfs_open(path, flags, &handle);
    if (ret < 0) {
        printf("[Open] %s %d %d\n", path, flags, ret);
        trapframe->x[0] = -1;
        return;
    }
    for (int i = 0; i < MAX_FD_NUM; i++) {
        if (!current_thread()->fd_table[i]) {
            current_thread()->fd_table[i] = handle;
            trapframe->x[0] = i;
            printf("[Open] %s %d %d\n", path, flags, i);
            return;
        }
    }
    trapframe->x[0] = -1;
    printf("[Open] %s %d %d\n", path, flags, trapframe->x[0]);
}

void sys_close(Trapframe *trapframe) {
    printf("Close\n");
    int fd = trapframe->x[0];
    if (fd < 0) {
        trapframe->x[0] = -1;
        return;
    }
    struct file *handle = current_thread()->fd_table[fd];
    int ret = vfs_close(handle);
    //current_thread()->fd_table[fd] = NULL;
    trapframe->x[0] = ret;
    printf("[Close] %d %d\n", fd, ret);
}

void sys_write(Trapframe *trapframe) {
    printf("[Write] %d\n", trapframe->x[0]);
    int fd = trapframe->x[0];
    char *buf = (char *)trapframe->x[1];
    unsigned long count = trapframe->x[2];
    if (fd < 0) {
        trapframe->x[0] = -1;
        printf("[Write] %d\n", trapframe->x[0]);
        return;
    }
    struct file *handle = current_thread()->fd_table[fd];
    if (handle == NULL) {
        trapframe->x[0] = 0;
        printf("[Write] %d %d\n", fd, trapframe->x[0]);
        return;
    }
    trapframe->x[0] = vfs_write(handle, buf, count);
    printf("[Write] %d %d\n", fd, trapframe->x[0]);
}

void sys_read(Trapframe *trapframe) {
    int fd = trapframe->x[0];
    char *buf = (char *)trapframe->x[1];
    unsigned long count = trapframe->x[2];
    printf("[Read] %d %d\n", fd, count);
    if (fd < 0) {
        trapframe->x[0] = -1;
        return;
    }
    struct file *handle = current_thread()->fd_table[fd];
    if (handle == NULL) {
        trapframe->x[0] = 0;
        return;
    }
    trapframe->x[0] = vfs_read(handle, buf, count);
    printf("[Read] %d\n", trapframe->x[0]);

}

void sys_mkdir(Trapframe *trapframe) {
    printf("Mkdir\n");
    char *pathname = trapframe->x[0];
    unsigned mode = trapframe->x[1];
    trapframe->x[0] = vfs_mkdir(pathname);
    printf("[Mkdir] %s %d\n", pathname, trapframe->x[0]);
}

void sys_mount(Trapframe *trapframe) {
    printf("Mount\n");
    char *src = trapframe->x[0];
    char *target = trapframe->x[1];
    char *filesystem = trapframe->x[2];
    unsigned long flags = trapframe->x[3];
    void *data = trapframe->x[4];
    trapframe->x[0] = vfs_mount(target, filesystem);
    printf("[Mount] %s %s %d\n", target, filesystem, trapframe->x[0]);
}

void sys_chdir(Trapframe *trapframe) {
    printf("Chdir\n");
    char *pathname = trapframe->x[0];
    trapframe->x[0] = vfs_chdir(pathname);
    printf("[Chdir] %s %d\n", pathname, trapframe->x[0]);
}

void timer_interrupt(int i) {
    unsigned long cntfrq_el0;
    asm volatile ("mrs %0, cntfrq_el0":"=r" (cntfrq_el0));
    asm volatile ("lsr %0, %0, #5":"=r" (cntfrq_el0) :"r"(cntfrq_el0)); // 1/32 second tick
    asm volatile ("msr cntp_tval_el0, %0" : : "r"(cntfrq_el0));
    timer_tick();
}