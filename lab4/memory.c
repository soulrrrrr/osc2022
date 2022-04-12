#include "memory.h"
#include "freelist.h"
#include "uart.h"
#include "utils.h"

Freelist heads[LOG2_MAX_PAGES_PLUS_1];
Node nodes[MAX_PAGES];
int frame_array[MAX_PAGES];

void memory_init() {
    for (int i = 0; i < MAX_PAGES; i++) {
        nodes[i].next = NULL;
        nodes[i].index = i;
        frame_array[i] = BELONG_LEFT;
    }
    frame_array[0] = LOG2_MAX_PAGES;
    for (int i = 0; i < LOG2_MAX_PAGES; i++) {
        heads[i].head = NULL;
    }
    heads[LOG2_MAX_PAGES].head = &nodes[0];
}

int find_allocate_list(Freelist *heads, int needed_pages) {
    for (int i = needed_pages; i < LOG2_MAX_PAGES; i++) {
        if (heads[i].head != NULL) {
            return i;
        }
    }
    return LOG2_MAX_PAGES;
}

int allocate_page(Freelist *heads, Node *nodes, int *frames, int needed_level, int use_level) {
    uart_puts("Allocate page for level ");
    uart_int(needed_level);
    uart_puts("\n");
    Node *fs = heads[use_level].head;
    int front = fs->index;
    freelist_remove(&heads[use_level], nodes, front);
    for (int i = use_level-1; i >= needed_level; i--) {
        int back = front | pow2(i);
        freelist_push(&heads[i], nodes, back);
        frames[back] = i;
    }
    frames[front] = ALLOCATED;
    for (int j = 0; j < 8; j++) {
        for (int i = 0; i < 16; i++) {
            uart_int(frames[16*j+i]);
            uart_puts(" ");
        }
        uart_puts("\n");
    }
    return fs->index;
}

void free_page(Freelist *heads, Node *nodes, int *frames, int free_index) {
    uart_puts("Free page: ");
    uart_int(free_index);
    uart_puts("\n");
    int level = 0;
    while(frames[free_index ^ pow2(level)] == BELONG_LEFT) { // 如果非最左邊區塊，終會到達index=0, frames[0]不可能BELONG_LEFT, 所以會跳出迴圈
        level++;
    }
    int free_level = LOG2_MAX_PAGES;
    int free_pages = MAX_PAGES;
    frames[free_index] = BELONG_LEFT;
    for (int i = level; i < LOG2_MAX_PAGES; i++) {
        int buddy = free_index ^ pow2(i);
        if (frames[buddy] != i) { // not same level or allocated
            free_level = i;
            free_pages = pow2(i);
            break;
        }
        frames[buddy] = BELONG_LEFT;
        freelist_remove(&heads[i], nodes, buddy);
        uart_puts("Merge ");
        uart_int(buddy);
        uart_puts("\n");
        free_index &= ~(pow2(i+1)-1);
    }
    freelist_push(&heads[free_level], nodes, free_index);
    uart_puts("Push to freelist ");
    uart_int(free_index);
    uart_puts("\n");
    frames[free_index] = free_level;

    for (int j = 0; j < 8; j++) {
        for (int i = 0; i < 16; i++) {
            uart_int(frames[16*j+i]);
            uart_puts(" ");
        }
        uart_puts("\n");
    }
    return;
}

void *malloc(int size) {
    if (size >= PAGE_SIZE) {
        int need_pages = (size+PAGE_SIZE-1)/PAGE_SIZE;
        uart_puts("Need ");
        uart_int(need_pages);
        uart_puts(" page(s)\n");
        int needed_order = log2((size+PAGE_SIZE-1)/PAGE_SIZE);
        int use_order = find_allocate_list(heads, needed_order);
        return (void *)(unsigned long)(MEMORY_BASE + allocate_page(heads, nodes, frame_array, needed_order, use_order) * PAGE_SIZE);
    }
    else {
        return (void *) 0x10000000; // dynamic allocator
    }
}

void free(void *ptr) {
    
    int free_index = ((unsigned long)ptr - MEMORY_BASE + (PAGE_SIZE-1)) / 0x1000;
    free_page(heads, nodes, frame_array, free_index);
    uart_puts("Free memory finished: ");
    uart_int(free_index);
    uart_puts("\n");
}