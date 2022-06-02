#ifndef UTILS_H
#define UTILS_H

#define DEBUG 0
int strcmp(const char *s1, const char *s2);
void strcpy(char *dest, const char *src);
int hex_to_int(char *p, int size);
void *get_user_program_address();
int log2(int x);
int pow2(int x);
unsigned long cstr_to_ulong(char *s);
void* simple_malloc(void **now, int size);
void debug(char *, int);
unsigned long get_timestamp();
void assert(int e);

int strlen(char *str);
#endif