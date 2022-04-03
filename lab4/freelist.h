#ifndef FREELISTS_H
#define FREELISTS_H

#define ALLOCABLE_NOT_START -1
#define ALLOCATED -2

struct Node {
    struct Node *next;
    int index;
};

struct Freelist {
    struct Node *first;
};

typedef struct Node Node;
typedef struct Freelist Freelist;

void freelist_init(Freelist *, Node *);
void freelist_push(Freelist *, Node *, int);
void freelist_remove(Freelist *, Node *, int);
void freelist_print(Freelist *);

#endif
