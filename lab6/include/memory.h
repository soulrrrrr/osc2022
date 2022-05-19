#ifndef MEMORY_H
#define MEMORY_H

#define MEMORY_BASE 0xFFFF000000000000
#define CPIO_SIZE 248320
#define PAGE_SIZE 0x1000UL // 4KB
#define MAX_PAGES 0x40000 // total 0x40000000
#define LOG2_MAX_PAGES 18
#define LOG2_MAX_PAGES_PLUS_1 19
#define NULL 0


#include "freelist.h"
#include "typedef.h"

struct block_meta {
    int size;
    short free;
    short pagetail;
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
void *malloc(size_t);
void free(void *);
void reserve_memory(ulong start, ulong end);
void print_memory();

#endif // _MEMORY_H
