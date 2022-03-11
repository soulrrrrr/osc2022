#include "uart.h"
#include "utils.h"

struct cpio_newc_header {
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
};

void main() {
    uart_init();
    uart_puts("Hello from Raspberry pi!\n");

    while (1) {
        uart_send('\r');
        uart_send('#');
        uart_send(' ');
        int i = 0;
        char input[1024];
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
        if (strcmp(input, "ls") == 0) {
            struct cpio_newc_header *fs = (struct cpio_newc_header *)0x8000000;
            char *current = (char *)0x8000000;
            while (1) {
                fs = (struct cpio_newc_header *)current;
                int name_size = hex_to_int(fs->c_namesize, 8);
                int file_size = hex_to_int(fs->c_filesize, 8);
                current += 110;
                // uart_hex((unsigned int)current);
                // uart_puts("\n");
                if (strcmp(current, "TRAILER!!!") == 0) {
                    // uart_puts("FInished ls\n");
                    break;
                }
                for (int i = 0; i < name_size; i++) {
                    if (*current == '\0') {
                        while ((current - (char *)fs) % 4 != 0)
                            current++;
                        break;
                    }
                    uart_send(*current);
                    current++;
                }
                uart_puts("\n");
                for (int i = 0; i < file_size; i++) {
                    if (*current == '\0') {
                        while ((current - (char *)fs) % 4 != 0)
                            current++;
                        break;
                    }
                    // uart_send(*current);
                    current++;
                }
                // uart_puts("\n");
            }

        } else
            uart_puts("Error input!\n");
    }
}
