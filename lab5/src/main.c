#include "uart.h"
#include "utils.h"
#include "freelist.h"
#include "memory.h"
#include "sched.h"
#include "printf.h"
#include "typedef.h"
#include "sys.h"
#include "mbox.h"
#define N 5

extern void delay();
extern void core_timer_enable();


void cpu_timer_register_enable() { // 讓 el0 用 clock 不會 interrupt
    uint64_t tmp;
    asm volatile("mrs %0, cntkctl_el1" : "=r"(tmp));
    tmp |= 1;
    asm volatile("msr cntkctl_el1, %0" : : "r"(tmp));
}

void fork_test(){
    printf("\nFork Test, pid %d\n", getpid());
    int cnt = 1;
    int ret = 0;
    if ((ret = fork()) == 0) { // child
        long long cur_sp;
        asm volatile("mov %0, sp" : "=r"(cur_sp));
        printf("first child pid: %d, cnt: %d, ptr: %x, sp : %x\n", getpid(), cnt, &cnt, cur_sp);
        ++cnt;
        if ((ret = fork()) != 0){
            asm volatile("mov %0, sp" : "=r"(cur_sp));
            printf("first child pid: %d, cnt: %d, ptr: %x, sp : %x\n", getpid(), cnt, &cnt, cur_sp);
        }
        else{
            while (cnt < 5) {
                asm volatile("mov %0, sp" : "=r"(cur_sp));
                printf("second child pid: %d, cnt: %d, ptr: %x, sp : %x\n", getpid(), cnt, &cnt, cur_sp);
                delay(1000000);
                ++cnt;
            }
        }
        exit(0);
    }
    else {
        unsigned int __attribute__((aligned(16))) mbox[36];
        get_board_revision(mbox);
        mbox_call(MBOX_CH_PROP, mbox);
        for (int i = 0; i < 8; i++) {
            printf("mbox %d: %x\n", i, mbox[i]);
        }
        printf("parent here, pid %d, child %d\n", getpid(), ret);
    }
}

void foo(){
    for(int i = 0; i < 10; ++i) {
        printf("Thread id: %d %d\n", getpid(), i);
        delay(10000000);
        schedule();
    }
}

void main() {
    uart_init();
    init_printf(NULL, putc);
    memory_init();
    task_init();
    core_timer_enable();
    cpu_timer_register_enable();
    while (1) {
        char c = uart_getc();
        uart_send(c);
        if (c == 's') break;
    }
    printf("%s", "\nHello from Raspberry pi!\n");
    // for(int i = 0; i < N; ++i) { // N should > 2
    //     thread_create(foo);
    // }
    // delay(1000000000);
    // kill_zombies();
    // printf("PID: %d\n", getpid());
    // for(int i = 0; i < N*2; ++i) { // N should > 2
    //     thread_create(foo);
    // }
    //fork_test();
    int ret;
    if ((ret = fork()) == 0)
        exec("syscall.img", 0x0);
    idle();
    // char input[1024];
    // while (1) {
    //     uart_send('\r');
    //     uart_puts("# ");
    //     shell_input(input);
    //     if (strcmp(input, "run") == 0) {
    //         uart_puts("running...\n");
    //     } else if (strcmp(input, "m") == 0) {
    //         shell_input(input);
    //         int size = (int)cstr_to_ulong(input);
    //         void *ptr = malloc(size);
    //         uart_puts("Allocation finished: ");
    //         uart_hex((uint)ptr);
    //         uart_puts("\n");
    //     } else if (strcmp(input, "d") == 0) {
    //         shell_input(input);
    //         void *ptr = (void *)(ulong)hex_to_int(input, 8);
    //         free(ptr);
    //         uart_puts("Free finished: ");
    //         uart_hex((uint)ptr);
    //         uart_puts("\n");
    //     } else if (strcmp(input, "pm") == 0) {
    //         print_freelists();
    //         print_memory();
    //     } else {
    //         uart_puts("Error input!\n");
    //     }
    // }
}
