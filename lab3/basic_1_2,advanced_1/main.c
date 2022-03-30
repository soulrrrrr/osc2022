#include "uart.h"
#include "utils.h"

int message_count = 0;

void main() {
    uart_init();

    uart_puts("Hello from Raspberry pi!\n");
    char input[1024];
    unsigned long el;

    asm volatile ("mrs %0, CurrentEL" : "=r" (el));

    uart_puts("Current EL is: ");
    uart_hex((el>>2)&3); // current el saved in bits[3:2]
    uart_puts("\n");
    struct message *queue_head = (struct message *) MESSAGE_QUEUE;
    while (1) {
        uart_send('\r');
        uart_puts("# ");
        shell_input(input);
        if (strcmp(input, "run") == 0) {
            uart_puts("running...\n");
            void *program_address = get_user_program_address();
            asm volatile ("mov x0, 0"); // 被 0x3c0 juke了, 3c0 是關 interrupt
            asm volatile ("msr spsr_el1, x0"); //Holds the saved process state when an exception is taken to EL1
            asm volatile ("msr elr_el1, %0": :"r" (program_address));
            asm volatile ("mov x0, 0x60000");
            asm volatile ("msr sp_el0, x0");
            asm volatile ("eret");
        } else if (strcmp(input, "st") == 0) {
            struct message *msg = (struct message *) MESSAGE_INSERT;
            uart_puts ("MESSAGE: ");
            shell_input(input);
            strcpy(msg->content, input);
            uart_puts ("SECONDS: ");
            shell_input(input);
            asm volatile ("msr DAIFSet, 0xf");
            unsigned long cntpct_el0, cntfrq_el0;
            asm volatile ("mrs %0, cntfrq_el0":"=r" (cntfrq_el0));
            asm volatile ("mrs %0, cntpct_el0":"=r" (cntpct_el0));
            msg->seconds = cntpct_el0 + cstr_to_ulong(input)*cntfrq_el0;
            sort_message(message_count);
            message_count++;
            // for (int i = 0; i < message_count; i++) {
            //     uart_ulong(((queue_head+i)->seconds));
            //     uart_puts("\n");
            // }
            asm volatile("mov x0, 1");
            asm volatile("msr cntp_ctl_el0, x0"); // disable
            asm volatile ("mrs %0, cntpct_el0":"=r" (cntpct_el0));
            asm volatile ("msr cntp_tval_el0, %0" : : "r"(queue_head->seconds - cntpct_el0));
            asm volatile ("msr DAIFClr, 0xf");
        } else {
            uart_puts("Error input!\n");
        }
    }
}
