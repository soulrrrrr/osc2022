#ifndef FREELISTS_H
#define FREELISTS_H

#define BELONG_LEFT -1
#define ALLOCATED -2
#define RESERVED -3

struct Node {
    struct Node *next;
    struct Node *prev;
    int index;
};

struct Freelist {
    struct Node *head;
};

typedef struct Node Node;
typedef struct Freelist Freelist;

void freelist_init(Freelist *, Node *);
void freelist_push(Freelist *, Node *, int);
void freelist_remove(Freelist *, Node *, int);
void freelist_print(int, Freelist *);
void print_freelists();

#endif
