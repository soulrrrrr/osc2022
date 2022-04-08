#include "memory.h"
#include "freelist.h"
#include "uart.h"
#include "utils.h"

int find_allocate_list(Freelist *heads, int needed_pages) {
    for (int i = needed_pages; i < LOG2_MAX_PAGES; i++) {
        if (heads[i].head != NULL) {
            return i;
        }
    }
    return LOG2_MAX_PAGES;
}

int allocate_memory(Freelist *heads, Node *nodes, int *frames, int *frame_levels, int needed_level, int use_level) {
    Node *fs = heads[use_level].head;
    int front = fs->index;
    freelist_remove(&heads[use_level], nodes, front);
    for (int i = use_level-1; i >= needed_level; i--) {
        int back = front | pow2(i);
        freelist_push(&heads[i], nodes, back);
        frames[back] = i;
    }
    for (int i = 0; i < pow2(needed_level); i++) {
        frames[front+i] = ALLOCATED;
    }
    frame_levels[front] = needed_level;
    for (int j = 0; j < 4; j++) {
        for (int i = 0; i < MAX_PAGES/4; i++) {
            uart_int(frames[16*j+i]);
            uart_puts(" ");
        }
        uart_puts("\n");
    }
    return fs->index;
}

void free_memory(Freelist *heads, Node *nodes, int *frames, int *frame_levels, int free_index) {
    int level = frame_levels[free_index];
    if (level < 0) return;
    int free_level = LOG2_MAX_PAGES;
    int free_pages = MAX_PAGES;
    for (int i = level; i < LOG2_MAX_PAGES; i++) {
        int buddy = free_index ^ pow2(i);
        if (frames[buddy] == ALLOCATED) {
            free_level = i;
            free_pages = pow2(i);
            break;
        }
        freelist_remove(&heads[i], nodes, buddy);
        uart_puts("Remove ");
        uart_int(buddy);
        uart_puts("\n");
        free_index &= ~(pow2(i+1)-1);
    }
    freelist_push(&heads[free_level], nodes, free_index);
    uart_puts("Push ");
    uart_int(free_index);
    uart_puts("\n");
    frames[free_index] = free_level;
    for (int i = 1; i < free_pages; i++) {
        frames[free_index+i] = ALLOCABLE_NOT_START;
    }
    for (int j = 0; j < 4; j++) {
        for (int i = 0; i < MAX_PAGES/4; i++) {
            uart_int(frames[16*j+i]);
            uart_puts(" ");
        }
        uart_puts("\n");
    }
    return;
}