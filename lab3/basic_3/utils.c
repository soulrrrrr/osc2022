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

void *get_user_program_address() {
    struct cpio_newc_header *fs = (struct cpio_newc_header *)0x8000000;
    char *current = (char *)0x8000000;
    int is_file = 0;
    while (1) {
        fs = (struct cpio_newc_header *)current;
        int name_size = hex_to_int(fs->c_namesize, 8);
        int file_size = hex_to_int(fs->c_filesize, 8);
        current += 110; // size of cpio_newc_header
        if (strcmp(current, "user.img") == 0)
            is_file = 1;
        current += name_size;
        while ((current - (char *)fs) % 4 != 0)
            current++;
        if (is_file) return (void *) current;
        current += file_size;
        while ((current - (char *)fs) % 4 != 0)
            current++;
    }

}

int strcmp(char *s1, char *s2) {
    while (*s1 != '\0' && *s1 == *s2) {
        s1++;
        s2++;
    }
    return (*(unsigned char *)s1) - (*(unsigned char *)s2);
}

int hex_to_int(char *p, int len) {
    int val = 0;
    int temp;
    for (int i = 0; i < len; i++) {
        temp = *(p + i);
        if (temp >= 'A') {
            temp = temp - 'A' + 10;
        } else
            temp -= '0';
        val *= 16;
        val += temp;
    }
    return val;
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

void exception_entry() {
    unsigned long spsrel1, elrel1, esrel1;
    asm volatile ("mrs %0, SPSR_EL1" : "=r" (spsrel1));
    uart_puts("SPSR_EL1: 0x");
    uart_hex_long(spsrel1);
    uart_puts("\n");
    asm volatile ("mrs %0, ELR_EL1" : "=r" (elrel1));
    uart_puts("ELR_EL1: 0x");
    uart_hex_long(elrel1);
    uart_puts("\n");
    asm volatile ("mrs %0, ESR_EL1" : "=r" (esrel1));
    uart_puts("ESR_EL1: 0x");
    uart_hex_long(esrel1);
    uart_puts("\n");
}


void print_core_timer(unsigned long frq, unsigned long cnt) {
    uart_puts("Core timer: ");
    uart_ulong(cnt/frq);
    uart_puts("\n");
}
