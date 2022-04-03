#include "freelist.h"
#include "uart.h"
#include "utils.h"

void freelist_push(Freelist *head, Node *nodes, int num) {
    if (!head->first) {
        head->first = &nodes[num];
        nodes[num].next = NULL;
        return;
    }
    Node *node = head->first;
    nodes[num].next = node;
    head->first = &nodes[num];
    return;
}

void freelist_remove(Freelist *list, Node *nodes, int num) {
    Node *prev = NULL;
    Node *current = list->first;
    // Walk the list
    while (current->index != num) {
        prev = current;
        current = current->next;
    }
    // Remove the target by updating the head or the previous node.
    if (!prev)
        list->first = current->next;
    else
        prev->next = current->next;
}

void freelist_print(Freelist *list) {
    uart_puts("List: ");
    for (Node *node = list->first; node != NULL; node = node->next) {
        uart_uint(node->index);
        uart_puts(" ");
    }
    uart_puts("\n");
}
