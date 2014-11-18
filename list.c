#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include "list.h"

/* your list function definitions */

void list_append(struct node *thread, struct node *head) {
   
	struct node *temp = head;
	thread->next =NULL;
	if (head == NULL){
		head = thread;
	}else{
		while(temp->next != NULL){
			temp = temp->next;
		}
		temp->next = thread;
	}
}

void list_clear(struct node *list) {
    while (list != NULL) {
        struct node *tmp = list;
        list = list->next;
        free(tmp);
    }
}
