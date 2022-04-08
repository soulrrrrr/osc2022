#include "freelist.h"
#include "uart.h"
#include "utils.h"

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
    if (!pre)
        list->head = current->next;
    else {
        current->next->prev = pre;
        pre->next = current->next;
    }
}

void freelist_print(Freelist *list) {
    uart_puts("List: ");
    for (Node *node = list->head; node != NULL; node = node->next) {
        uart_uint(node->index);
        uart_puts(" ");
    }
    uart_puts("\n");
}
