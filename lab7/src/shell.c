#include "utils.h"
#include "uart.h"
#include "printf.h"
#include "memory.h"
#include "vfs.h"
#include "tmpfs.h"
#include "shell.h"
#include "sched.h"
#include "sys.h"

void shell() {
    printf("\n\n _  _  ___ _____ _   _  ___  ___ ___ ___ \n");
    printf("| \\| |/ __|_   _| | | |/ _ \\/ __|   \\_ _|\n");
    printf("| .` | (__  | | | |_| | (_) \\__ \\ |) | | \n");
    printf("|_|\\_|\\___| |_|  \\___/ \\___/|___/___/___|\n\n");
    char input[1024];
    while (1) {
        uart_send('\r');
        uart_puts("# ");
        shell_input(input);
        if (strcmp(input, "test") == 0) {
            struct vnode *vnode;
            int fd = open("/dir", O_CREAT);
            int ret = write(fd, "abcdefghijklmnopqrstuvwxyz", 26);
            printf("%d %d\n", fd, ret);
            fd = open("/dir", 0);
            char buf[128];
            int rd = read(fd, buf, 10);
            printf("%d %d %s\n", fd, rd, buf);
            ret = vfs_lookup(".", &vnode);
            printf("Current directory: %s\n", ((struct tmpfs_internal *)vnode->internal)->name);


        } else if (strcmp(input, "m") == 0) {
            shell_input(input);
            int size = (int)cstr_to_ulong(input);
            void *ptr = malloc(size);
            printf("%x\n", ptr);
        } else if (strcmp(input, "d") == 0) {
            shell_input(input);
            void *ptr = (void *)(ulong)hex_to_int(input, 8);
            free(ptr);
        } else if (strcmp(input, "pm") == 0) {
            print_freelists();
            print_memory();
        } else if (!strcmp(input, "pwd")) {
            printf("%s\n", ((struct tmpfs_internal *)current_thread()->pwd->internal)->name);
        } else if (!strcmp(input, "vfs1")) {
            int ret;
            char *argv[] = {};
            if ((ret = fork()) == 0)
                exec("vfs1.img", argv);
            idle();
        } else {
            uart_puts("Error input!\n");
        }
    }

}

void shell_input(char *input) {
    int i = 0;
    char temp;
    while (1) {
        temp = uart_getc();
        if (temp == '\n') {
            uart_puts("\n");
            input[i] = '\0';
            break;
        } else
            uart_send(temp);

        input[i] = temp;
        i++;
    }
}