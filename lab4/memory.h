#ifndef MEMORY_H
#define MEMORY_H

#define MEMORY_BASE 0x10000000
#define PAGE_SIZE 0x1000 // 4KB
#define MAX_PAGES 128
#define LOG2_MAX_PAGES 7
#define LOG2_MAX_PAGES_PLUS_1 8

#include "freelist.h"
void memory_init(void);
int find_allocate_list(Freelist *, int);
int allocate_page(Freelist *, Node *, int *, int, int);
void free_page(Freelist *, Node *, int *, int);
void *malloc(int);
void free(void *);
#endif
