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

void main() {
    uart_init();

    uart_puts("Hello from Raspberry pi!\n");
    void *memory = (void *)0x120000;
    char input[1024];
    char *s = simple_malloc(&memory, 8);
    uart_puts("Allocated 8 bytes of memory\n");
    uart_hex((unsigned int)s);
    uart_puts("\n");
    s = simple_malloc(&memory, 34);
    uart_puts("Allocated 34 bytes of memory\n");
    uart_hex((unsigned int)s);
    uart_puts("\n");
    s = simple_malloc(&memory, 3);
    uart_puts("Allocated 3 bytes of memory\n");
    uart_hex((unsigned int)s);
    uart_puts("\n");

    while (1) {
        uart_send('\r');
        uart_puts("# ");
        shell_input(input);

        if (strcmp(input, "ls") == 0) {
            struct cpio_newc_header *fs = (struct cpio_newc_header *)0x8000000;
            char *current = (char *)0x8000000;
            while (1) {
                fs = (struct cpio_newc_header *)current;
                int name_size = hex_to_int(fs->c_namesize, 8);
                int file_size = hex_to_int(fs->c_filesize, 8);
                current += 110; // size of cpio_newc_header
                if (strcmp(current, "TRAILER!!!") == 0)
                    break;
                uart_puts(current);
                uart_puts("\n");
                current += name_size;
                while ((current - (char *)fs) % 4 != 0)
                    current++;
                current += file_size;
                while ((current - (char *)fs) % 4 != 0)
                    current++;
            }

        } else if (strcmp(input, "cat") == 0) {
            uart_puts("Filename: ");
            shell_input(input);
            struct cpio_newc_header *fs = (struct cpio_newc_header *)0x8000000;
            char *current = (char *)0x8000000;
            while (1) {
                fs = (struct cpio_newc_header *)current;
                int name_size = hex_to_int(fs->c_namesize, 8);
                int file_size = hex_to_int(fs->c_filesize, 8);
                current += 110;
                if (strcmp(current, "TRAILER!!!") == 0) {
                    uart_puts("No such file!\n");
                    break;
                }

                if (strcmp(current, input) == 0) {
                    current += name_size;
                    while ((current - (char *)fs) % 4 != 0)
                        current++;
                    for (int i = 0; i < file_size; i++) {
                        uart_send(*current);
                        current++;
                    }
                    uart_puts("\n");
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
        } else {
            uart_puts("Error input!\n");
        }
    }
}
