#include "uart.h"
#include "utils.h"
#include "freelist.h"
#include "memory.h"
#include "sched.h"
#include "printf.h"
#define N 5

extern void delay();
extern void enable_irq();

// void fork_test(){
//     printf("\nFork Test, pid %d\n", get_pid());
//     int cnt = 1;
//     int ret = 0;
//     if ((ret = fork()) == 0) { // child
//         long long cur_sp;
//         asm volatile("mov %0, sp" : "=r"(cur_sp));
//         printf("first child pid: %d, cnt: %d, ptr: %x, sp : %x\n", get_pid(), cnt, &cnt, cur_sp);
//         ++cnt;

//         if ((ret = fork()) != 0){
//             asm volatile("mov %0, sp" : "=r"(cur_sp));
//             printf("first child pid: %d, cnt: %d, ptr: %x, sp : %x\n", get_pid(), cnt, &cnt, cur_sp);
//         }
//         else{
//             while (cnt < 5) {
//                 asm volatile("mov %0, sp" : "=r"(cur_sp));
//                 printf("second child pid: %d, cnt: %d, ptr: %x, sp : %x\n", get_pid(), cnt, &cnt, cur_sp);
//                 delay(1000000);
//                 ++cnt;
//             }
//         }
//         exit();
//     }
//     else {
//         printf("parent here, pid %d, child %d\n", get_pid(), ret);
//     }
// }

void foo(){
    for(int i = 0; i < 10; ++i) {
        printf("Thread id: %d %d\n", current_thread()->pid, i);
        delay(10000000);
        schedule();
    }
}

void main() {
    init_printf(NULL, putc);
    uart_init();
    memory_init();
    while (1) {
        char c = uart_getc();
        uart_send(c);
        if (c == 's') break;
    }
	enable_irq();
    printf("%s", "\nHello from Raspberry pi!\n");
    for(int i = 0; i < N; ++i) { // N should > 2
        thread_create(foo);
    }
    delay(1000000000);
    kill_zombies();
    for(int i = 0; i < N*2; ++i) { // N should > 2
        thread_create(foo);
    }
    //fork_test();
    idle();
    // char input[1024];
    // while (1) {
    //     uart_send('\r');
    //     uart_puts("# ");
    //     shell_input(input);
    //     if (strcmp(input, "run") == 0) {
    //         uart_puts("running...\n");
    //     } else if (strcmp(input, "pm") == 0) {
    //         print_freelists();
    //         print_memory();
    //     } else {
    //         uart_puts("Error input!\n");
    //     }
    // }
}
