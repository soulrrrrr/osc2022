#include "memory.h"
#include "freelist.h"
#include "uart.h"
#include "utils.h"

//Freelist heads[LOG2_MAX_PAGES_PLUS_1];
// Node nodes[MAX_PAGES];
// int frame_array[MAX_PAGES];
// blocklist memory_blocks;

Freelist *heads;
Node *nodes;
int *frame_array;
blocklist memory_blocks;

extern char _end;

void memory_init() {
    void *base = (void *)&_end;
    heads = (Freelist *)simple_malloc(&base, sizeof(Freelist)*LOG2_MAX_PAGES_PLUS_1);
    nodes = (Node *)simple_malloc(&base, sizeof(Node)*MAX_PAGES);
    frame_array = (int *)simple_malloc(&base, sizeof(int)*MAX_PAGES);

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
    
    reserve_memory(0x0, (int)base);
    reserve_memory(0x8000000, (int)(0x8000000+CPIO_SIZE));
    reserve_memory(0x3c000000, 0x40000000);

    memory_blocks.head = (block_meta *)malloc(PAGE_SIZE);
    memory_blocks.head->next = NULL;
    memory_blocks.head->size = 4096 - BLOCK_SIZE;
    memory_blocks.head->free = 1;
    memory_blocks.head->pagetail = 1;

}

int find_allocate_list(Freelist *heads, int needed_pages) {
    for (int i = needed_pages; i <= LOG2_MAX_PAGES; i++) {
        if (heads[i].head != NULL) {
            return i;
        }
    }
    return LOG2_MAX_PAGES;
}

int allocate_page(Freelist *heads, Node *nodes, int *frames, int needed_level, int index) {
    uart_puts("\nAllocate page for level ");
    uart_int(needed_level);
    uart_puts("\n");
    if (index >= 0) {
        // reserve memory
        int remove = index, push;
        int use_level = needed_level;
        while(frames[remove] != use_level) {
            remove &= ~(pow2(use_level));
            use_level++;
        }
        freelist_remove(&heads[use_level], nodes, remove);
        debug("Remove", remove);
        for(int i = use_level-1; i >= needed_level; i--) {
            debug("i", i);
            remove ^= (index & pow2(i));
            push = remove ^ pow2(i);
            freelist_push(&heads[i], nodes, push);
            frames[push] = i;
            debug("Push", push);
        }
        frames[index] = ALLOCATED;
        return 0; // no need to return anything
    }
    else {
        // normal page allocation
        int use_level = find_allocate_list(heads, needed_level);
        Node *fs = heads[use_level].head;
        int front = fs->index;
        freelist_remove(&heads[use_level], nodes, front);
        debug("Remove from freelist", front);
        for (int i = use_level-1; i >= needed_level; i--) {
            int back = front | pow2(i);
            freelist_push(&heads[i], nodes, back);
            debug("Push to freelist", back);
            frames[back] = i;
        }
        frames[front] = ALLOCATED;
        return fs->index;
    }
}

void free_page(Freelist *heads, Node *nodes, int *frames, int free_index) {
    debug("Freeing page", free_index);
    frames[free_index] = BELONG_LEFT;
    int level = 0;
    int free_level = LOG2_MAX_PAGES;
    while(frames[free_index ^ pow2(level)] == BELONG_LEFT) { // 如果非最左邊區塊，終會到達index=0, frames[0]不可能BELONG_LEFT, 所以會跳出迴圈
        free_index &= ~(pow2(level));
        level++;
    }
    for (int i = level; i < LOG2_MAX_PAGES; i++) {
        int buddy = free_index ^ pow2(i);
        if (frames[buddy] != i) { // not same level or allocated
            free_level = i;
            break;
        }
        frames[buddy] = BELONG_LEFT;
        freelist_remove(&heads[i], nodes, buddy);
        debug("Merged", buddy);
        free_index &= ~(pow2(i));
    }
    freelist_push(&heads[free_level], nodes, free_index);
    debug("Push back to freelist", free_index);
    frames[free_index] = free_level;
    return;
}

void *malloc(int size) {
    if (size >= (int)(PAGE_SIZE-BLOCK_SIZE)) {
        int need_pages = (size+PAGE_SIZE-1)/PAGE_SIZE;
        uart_puts("Need ");
        uart_int(need_pages);
        uart_puts(" page(s)\n");
        int needed_order = log2((size+PAGE_SIZE-1)/PAGE_SIZE);
        void *ptr = (void *)(unsigned long)(MEMORY_BASE + allocate_page(heads, nodes, frame_array, needed_order, -1) * PAGE_SIZE);
        //print_freelists();
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
                new_page->pagetail = 1;
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
        new_block->size = left_size;
        new_block->free = 1;
        new_block->pagetail = curr->pagetail;
        new_block->next = curr->next;
        curr->size = size;
        curr->free = 0;
        curr->pagetail = 0;
        curr->next = new_block;
        return (void *)((ulong)curr+BLOCK_SIZE);

    }
}

void free(void *ptr) {
    if ((ulong)ptr % PAGE_SIZE == 0) {
        int free_index = ((unsigned long)ptr - MEMORY_BASE + (PAGE_SIZE-1)) / 0x1000;
        free_page(heads, nodes, frame_array, free_index);
        //print_freelists();
    }
    else {
        block_meta *need_free = (block_meta *)((ulong)ptr-BLOCK_SIZE);
        need_free->free = 1;
        /* remove block */
        block_meta *curr = memory_blocks.head;
        while(curr != NULL) {
            if (curr->free) {
                while(!curr->pagetail && (curr->next != NULL) && curr->next->free) {
                    curr->size += curr->next->size;
                    curr->pagetail = curr->next->pagetail;
                    curr->next = curr->next->next;
                }
            }
            curr = curr->next;
        }
    }
}

void reserve_memory(int start, int end) {
    int index = (start-MEMORY_BASE) / PAGE_SIZE;
    int pages = ((end+PAGE_SIZE-1)-start) / PAGE_SIZE;
    uart_puts("Pages: ");
    uart_int(pages);
    uart_puts("\n");
    for (int i = 0; i < LOG2_MAX_PAGES; i++) {
        if (index & pow2(i)) {
            debug("allocate", i);
            allocate_page(heads, nodes, frame_array, i, index);
            index += pow2(i);
            pages -= pow2(i); 
        }
        if (pages <= 0) break;
        if (pow2(i) >= pages) {
            allocate_page(heads, nodes, frame_array, log2(pages), index);
            break;
        }
        if (pages <= 0) break;
    }
    uart_puts("[Reserve memory] Finished.\n");
    print_freelists();
}

void print_memory() {
    block_meta *curr = memory_blocks.head;
    while(curr != NULL) {
        uart_puts("----------------\n");
        uart_puts("Address: ");
        uart_hex((ulong)curr + BLOCK_SIZE);
        uart_puts("\n");
        uart_puts("Size: ");
        uart_int(curr->size);
        uart_puts("\n");
        uart_puts("Free: ");
        if (curr->free)
            uart_puts("Yes\n");
        else
            uart_puts("No\n");
        uart_puts("Last block in the page: ");
        if (curr->pagetail)
            uart_puts("Yes\n");
        else
            uart_puts("No\n");

        curr = curr->next;
    }
}