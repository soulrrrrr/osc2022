#ifndef UTILS_H
#define UTILS_H

#define uint unsigned int
#define ulong unsigned long
int strcmp(char *s1, char *s2);
int hex_to_int(char *p, int size);
void *get_user_program_address();
void shell_input(char *input);
int log2(int x);
int pow2(int x);
unsigned long cstr_to_ulong(char *s);

#endif