#include "uart.h"
#include "utils.h"
#include "printf.h"

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

int strcmp(const char *s1, const char *s2) {
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

void* simple_malloc(void **now, int size) {
    void *ret = *now;
    *now = *(char **)now + size;
    return ret;
}

int log2(int x) {
    int ret = 0;
    while(x != 1) {
        if (x & 1) {
            x += 1;
        }
        x >>= 1;
        ret++;
    }
    return ret;
}

int pow2(int x) {
    return (1 << x);
}

unsigned long cstr_to_ulong(char *s) {
    unsigned long ret = 0;
    while (*s != '\0') {
        ret *= 10;
        ret += (*s - '0');
        s++;
    }
    return ret;
}

void strcpy(char *dest, const char *src) {
    while (*src != '\0') {
        *dest = *src;
        src++;
        dest++;
    }
    *dest = *src;
}

void debug(char *s, int n) {
    if (!DEBUG) return;
    uart_puts(s);
    uart_puts(": ");
    uart_int(n);
    uart_puts("\n");
}

unsigned long get_timestamp() {
    register unsigned long f, c;
    asm volatile ("mrs %0, cntfrq_el0" : "=r"(f)); // get current counter frequency
    asm volatile ("mrs %0, cntpct_el0" : "=r"(c)); // read current counter
    return (unsigned long) c;
}

void assert(int e) {
    if (e == 0)
        printf("Assert Error!\n");
    else
        printf("Assert\n");
}

int strlen(char *str) {
    int len = 0;
    while (*str != '\0') {
        len++;
        str++;
    }
    return len;
}