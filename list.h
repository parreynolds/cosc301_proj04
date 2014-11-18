#ifndef __LIST_H__
#define __LIST_H__

#include <ucontext.h>

/* your list data structure declarations */

struct node {
    ucontext_t ctx;
    struct node *next; 
};

/* your function declarations associated with the list */

void list_append(struct node *thread, struct node *head);
void list_clear(struct node *list);

#endif // __LIST_H__
