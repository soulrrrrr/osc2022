#include "uart.h"
#include "utils.h"
#include "freelist.h"
#include "memory.h"

Freelist heads[LOG2_MAX_PAGES_PLUS_1];
Node nodes[MAX_PAGES];
int frame_array[MAX_PAGES];
int frame_level_array[MAX_PAGES];

void main() {
    uart_init();
    for (int i = 0; i < MAX_PAGES; i++) {
        nodes[i].next = NULL;
        nodes[i].index = i;
        frame_array[i] = ALLOCABLE_NOT_START;
        frame_level_array[i] = -1;
    }
    frame_array[0] = LOG2_MAX_PAGES;
    for (int i = 0; i < LOG2_MAX_PAGES; i++) {
        heads[i].head = NULL;
    }
    heads[LOG2_MAX_PAGES].head = &nodes[0];
    uart_puts("Hello from Raspberry pi!\n");
    char input[1024];
    while (1) {
        uart_send('\r');
        uart_puts("# ");
        shell_input(input);
        if (strcmp(input, "run") == 0) {
            uart_puts("running...\n");
        } else if (strcmp(input, "m") == 0) {
            shell_input(input);
            int size = (int)cstr_to_ulong(input);
            int needed_order = log2(size);
            int use_order = find_allocate_list(heads, needed_order);
            int block_index = allocate_memory(heads, nodes, frame_array, frame_level_array, needed_order, use_order);
            for (int i = LOG2_MAX_PAGES; i >= 0; i--) {
                freelist_print(&heads[i]);
            }
            uart_puts("Allocation finished: ");
            uart_int(block_index);
            uart_puts("\n");
        } else if (strcmp(input, "d") == 0) {
            shell_input(input);
            int free_index = (int)cstr_to_ulong(input);
            free_memory(heads, nodes, frame_array, frame_level_array, free_index);
            for (int i = LOG2_MAX_PAGES; i >= 0; i--) {
                freelist_print(&heads[i]);
            }
            uart_puts("Free memory finished: ");
            uart_int(free_index);
            uart_puts("\n");
        } else {
            uart_puts("Error input!\n");
        }
    }
}
