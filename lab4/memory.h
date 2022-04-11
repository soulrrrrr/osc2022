#ifndef MEMORY_H
#define MEMORY_H

#define MEMORY_BASE 0x10000000
#define PAGESIZE 0x1000 // 4KB
#define MAX_PAGES 64
#define LOG2_MAX_PAGES 6
#define LOG2_MAX_PAGES_PLUS_1 7

#include "freelist.h"
int find_allocate_list(Freelist *, int);
int allocate_page(Freelist *, Node *, int *, int *, int, int);
void free_page(Freelist *, Node *, int *, int *, int);

#endif
