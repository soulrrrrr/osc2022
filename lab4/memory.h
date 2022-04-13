#ifndef MEMORY_H
#define MEMORY_H

#define MEMORY_BASE 0x10000000
#define PAGE_SIZE 0x1000 // 4KB
#define MAX_PAGES 128
#define LOG2_MAX_PAGES 7
#define LOG2_MAX_PAGES_PLUS_1 8
#define NULL ((void *)0)

#include "freelist.h"

struct block_meta {
    int size;
    int free;
    struct block_meta *next;
};

struct blocklist {
    struct block_meta *head;
};

typedef struct block_meta block_meta;
typedef struct blocklist blocklist;
#define BLOCK_SIZE (sizeof(block_meta))

void memory_init(void);
int find_allocate_list(Freelist *, int);
int allocate_page(Freelist *, Node *, int *, int, int);
void free_page(Freelist *, Node *, int *, int);
void *malloc(int);
void free(void *);
#endif
