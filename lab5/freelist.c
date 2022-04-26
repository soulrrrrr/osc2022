#include "memory.h"
#include "freelist.h"
#include "uart.h"
#include "utils.h"

extern Freelist *heads;
extern int *frame_array;

void freelist_push(Freelist *list, Node *nodes, int num) {
    if (!list->head) {
        list->head = &nodes[num];
        nodes[num].next = NULL;
        nodes[num].prev = NULL;
        return;
    }
    Node *node = list->head;
    node->prev = &nodes[num];
    nodes[num].next = node;
    list->head = &nodes[num];
    return;
}

void freelist_remove(Freelist *list, Node *nodes, int num) {
    Node *current = &nodes[num];
    Node *pre = current->prev;
    // Remove the target by updating the head or the previous node.
    if (pre==NULL)
        list->head = current->next;
    else {
        if (current->next == NULL) {
            pre->next = current->next;
        }
        else {
            current->next->prev = pre;
            pre->next = current->next;
        }
    }
}

void print_freelists() {
    uart_puts("-----------Freelists------------\n");
    for(int i = LOG2_MAX_PAGES; i >= 0; i--) {
        freelist_print(i, &heads[i]);
    }
    // uart_puts("-------------Pages--------------\n");
    // for (int j = 0; j < (MAX_PAGES/16); j++) {
    //     for (int i = 0; i < 16; i++) {
    //         uart_int(frame_array[16*j+i]);
    //         uart_puts(" ");
    //     }
    //     uart_puts("\n");
    // }
}
void freelist_print(int level, Freelist *list) {
    uart_puts("Level ");
    uart_int(level);
    uart_puts(": ");
    for (Node *node = list->head; node != NULL; node = node->next) {
        uart_uint(node->index);
        uart_puts(" ");
    }
    uart_puts("\n");
}
