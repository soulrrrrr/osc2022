#include "memory.h"
#include "freelist.h"
#include "uart.h"
#include "utils.h"

Freelist heads[LOG2_MAX_PAGES_PLUS_1];
Node nodes[MAX_PAGES];
int frame_array[MAX_PAGES];
blocklist memory_blocks;

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

    memory_blocks.head = (block_meta *)malloc(PAGE_SIZE);
    memory_blocks.head->next = NULL;
    memory_blocks.head->size = 4096 - BLOCK_SIZE;
    memory_blocks.head->free = 1;
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
    frames[free_index] = BELONG_LEFT;
    int level = 0;
    int free_level = LOG2_MAX_PAGES;
    int free_pages = MAX_PAGES;
    while(frames[free_index ^ pow2(level)] == BELONG_LEFT) { // 如果非最左邊區塊，終會到達index=0, frames[0]不可能BELONG_LEFT, 所以會跳出迴圈
        free_index &= ~(pow2(level));
        level++;
    }
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
        free_index &= ~(pow2(i));
        // free_index &= ~(pow2(i+1)-1);
    }
    freelist_push(&heads[free_level], nodes, free_index);
    uart_puts("Push to freelist ");
    uart_int(free_index);
    uart_puts("\n");
    frames[free_index] = free_level;

    for (int j = 0; j < (MAX_PAGES/16); j++) {
        for (int i = 0; i < 16; i++) {
            uart_int(frames[16*j+i]);
            uart_puts(" ");
        }
        uart_puts("\n");
    }
    return;
}

void *malloc(int size) {
    if (size >= (int)(PAGE_SIZE-BLOCK_SIZE)) {
        int need_pages = (size+PAGE_SIZE-1)/PAGE_SIZE;
        uart_puts("Need ");
        uart_int(need_pages);
        uart_puts(" page(s)\n");
        int needed_order = log2((size+PAGE_SIZE-1)/PAGE_SIZE);
        int use_order = find_allocate_list(heads, needed_order);
        void *ptr = (void *)(unsigned long)(MEMORY_BASE + allocate_page(heads, nodes, frame_array, needed_order, use_order) * PAGE_SIZE);
        print_freelists();
        return ptr;
    }
    else {
        uart_puts("Dynamic allocation\n");
        block_meta *curr = memory_blocks.head;
        /* find split block */
        while(1) {
            if (curr->free && curr->size > size) {
                break;
            }
            if (curr->next == NULL) {
                /* allocate new page */
                block_meta *new_page = malloc(PAGE_SIZE);
                new_page->size = PAGE_SIZE-BLOCK_SIZE;
                new_page->free = 1;
                new_page->next = NULL;
                curr->next = new_page;
                curr = curr->next;
                break;
            } 
            curr = curr->next;
        }
        /* allocate memory */
        int left_size = curr->size - size;
        block_meta *new_block = (block_meta *)((ulong)curr+BLOCK_SIZE+(ulong)size);
        uart_hex(new_block);
        uart_puts("\n");
        new_block->size = left_size;
        new_block->free = 1;
        new_block->next = curr->next;
        curr->size = size;
        curr->free = 0;
        curr->next = new_block;
        return (void *)((ulong)curr+BLOCK_SIZE);

    }
}

void free(void *ptr) {
    if ((ulong)ptr % PAGE_SIZE == 0) {
        int free_index = ((unsigned long)ptr - MEMORY_BASE + (PAGE_SIZE-1)) / 0x1000;
        free_page(heads, nodes, frame_array, free_index);
        print_freelists();
        uart_puts("Freed page index: ");
        uart_int(free_index);
        uart_puts("\n");
    }
    else {
        block_meta *need_free = (block_meta *)((ulong)ptr-BLOCK_SIZE);
        need_free->free = 1;
        /* remove block */
        block_meta *curr = memory_blocks.head;
        while(curr != NULL) {
            if (curr->free) {
                while((curr->next != NULL) && curr->next->free) {
                    curr->size += curr->next->size;
                    curr->next = curr->next->next;
                }
            }
            curr = curr->next;
        }
    }
}