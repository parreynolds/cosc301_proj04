/*
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <strings.h>
#include <string.h>
#include <ucontext.h>
#include "list.c"
#include "list.h"
#include "threadsalive.h"
/* ***************************** 
     stage 1 library functions
   ***************************** */
ucontext_t main_ctx; //global variabe will not be placed in queue used for swapping purposes
struct node* list;
int destroyed_threads = 0;


void ta_libinit(void) {
	list = NULL;
//	getcontext(&main_ctx);
//      list = malloc(sizeof(struct node));
    return;
}


void ta_create(void (*func)(void *), void *arg) {
    struct node *new_thread = malloc(sizeof(struct node));
    new_thread->next = NULL;
    unsigned char *stack = (unsigned char *)malloc(128000);
   // ucontext_t ctx = new_thread->ctx;
    getcontext(&new_thread->ctx); //initiliaze context
    new_thread->ctx.uc_stack.ss_sp = stack;
    new_thread->ctx.uc_stack.ss_size = 128000;
    new_thread->ctx.uc_link = &main_ctx;
    makecontext(&new_thread->ctx, (void (*) (void)) func, 1, arg); //thread goes to func
    if (list == NULL){
	list = new_thread;
    }
    else{
	new_thread->next = NULL;
	list_append(new_thread, list);
   }
    return;
}
void ta_yield(void) {
    	if(list == NULL){
		return;
    	}
   	struct node *temp = list;
    	list = list->next;
	temp->next = NULL;
	list_append(temp, list);
    	swapcontext(&temp->ctx, &list->ctx);
    	return;
}
int ta_waitall(void) {

	while(list!=NULL){
		swapcontext(&main_ctx, &list->ctx);
		if(list!=NULL){
			struct node *temp = list;
			list = list->next;
			free(temp->ctx.uc_stack.ss_sp);
			free(temp);
		}
	}
	if(destroyed_threads!=0){
		return -1;
	}
	else{
		return 0;
	}

 /*   struct node *current_node = list;
    while(current_node != NULL){
//	printf("here\n");
	swapcontext(&main_ctx, &list->ctx);
	current_node = current_node->next;
    }
    while(list!=NULL){
	struct node *temp = list;
 	list = list->next;
	free(temp->ctx.uc_stack.ss_sp);
	free(temp);
    }
    return 0;
*/
}
/* ***************************** 
     stage 2 library functions
   ***************************** */


void ta_sem_init(tasem_t *sem, int value) {
    sem->count = value;
    sem->sem_list = NULL;
}


void ta_sem_destroy(tasem_t *sem) {
    	while(sem->sem_list != NULL){
		struct node *temp = sem->sem_list;
		sem->sem_list = sem->sem_list->next;
		free(temp->ctx.uc_stack.ss_sp);
		free(&temp->ctx);
		free(temp);
		destroyed_threads++;
	}
}


void ta_sem_post(tasem_t *sem) {
	//if there is a thread in blocked queue move it to front of ready queue, move current thread to end of ready queue, swap from current to the front of ready queue
	//if theres nothing on the semaphore queue then change counter and keep running

	if(sem->count > 0 && sem->sem_list!=NULL){
		//move current thread to end of ready list
		struct node *head = list;
		list = list->next;
		head->next = NULL;
		list_append(head, list);
		// move front of blocked queue to front of ready queue
		struct node *temp = sem->sem_list;
		sem->sem_list = sem->sem_list->next;
		temp->next = list;
		list = temp; 
		//swap from current (now on back of ready queue) to front of queue
		swapcontext(&head->ctx, &list->ctx);
	}
	//the sem count is incremented regardless
	sem->count++;
}

/*
	sem->count++; //increment counter
	if(sem->sem_list == NULL){
		return; //if list is empty do nothing
	}
	//append current head of list to the end
	struct node *head = sem->sem_list;
	sem->sem_list = head->next;
	struct node *temp = list;
	while(temp->next != NULL){
		temp = temp->next;
	}
	temp->next = head;
	head->next = NULL;
	free(temp);
	free(head);
	destroyed_threads++;	
*/

/*
	sem->count += 1;
	if((sem->count <= 0) && (sem->sem_list != NULL)){
	   swapcontext(&main_ctx, &sem->sem_list->ctx);
	}
*/

void ta_sem_wait(tasem_t *sem) {
	//if value > 0 then decrement and keep running
	//if value of semaphore is 0 then you need to add yourself to the semqueue and take off ready queue context switch to new head of ready queue
	
	if(sem->count ==0){
		//take first node off of ready queue
		struct node *thread = list;
		list = list->next;
		//add current node to end of bloked queue
		thread->next = NULL;
		list_append(thread, sem->sem_list);
		//swap context to new head of redy queue
		swapcontext(&thread->ctx, &list->ctx);
	}
	sem->count --;


/*

	if(sem->sem_list == NULL){
		return; //what do we do? just return	
	}

	struct node *head = list;
	if(sem->count <= 0){
		list = head->next;
		head->next = NULL;
		struct node *temp = sem->sem_list;
		if(temp == NULL){
			sem->sem_list = head;	
		}
		else{
			while(temp->next != NULL){
				temp = temp->next;
			}
			temp->next = head;
		}
		free(temp);
		free(head);
		destroyed_threads++;
		ta_yield();
	}

	sem->count--;
*/

/*
	if (sem->count > 0){
	   sem->count -=1;
	}
	if (sem == 0){ //what does this mean? why not sem->count?
	   swapcontext(&main_ctx, &list->ctx);
	}
*/
}
void ta_lock_init(talock_t *mutex) {
	mutex->sem = *(tasem_t *)malloc(sizeof(tasem_t));
	ta_sem_init(&mutex->sem, 1);
}
void ta_lock_destroy(talock_t *mutex) {
    	ta_sem_destroy(&mutex->sem);
	//free(mutex);
}
void ta_lock(talock_t *mutex) {
	ta_sem_wait(&mutex->sem);
}
void ta_unlock(talock_t *mutex) {
	ta_sem_post(&mutex->sem);
}
/* ***************************** 
     stage 3 library functions
   ***************************** */
void ta_cond_init(tacond_t *cond) {
	//cond->sem = *(tasem_t *)malloc(sizeof(tasem_t));
	//ta_sem_init(&cond->sem, 0);
	cond->cond_list = NULL;
}
void ta_cond_destroy(tacond_t *cond) {
/*
    struct node *temp = cond->sem.sem_list;
    while(cond->sem.sem_list->next != NULL){
	temp = cond->sem.sem_list;
	cond->sem.sem_list = cond->sem.sem_list->next;
	free(temp);
        destroyed_threads ++;
    }
   free(cond->sem.sem_list);
*/
	while(cond->cond_list != NULL){
		struct node *temp = cond->cond_list;
		cond->cond_list = cond->cond_list->next;
		free(temp->ctx.uc_stack.ss_sp);
		free(&temp->ctx);
		free(temp);
		destroyed_threads++;
	}

}
void ta_wait(talock_t *mutex, tacond_t *cond) {

	if(mutex==NULL || cond==NULL){
		return;
	}
	ta_unlock(mutex);
	//get first element of list
	struct node *current = list;
	list = list->next;
	//append that element to cond_list
	current->next = NULL;
	list_append(current, cond->cond_list);
	if(list == NULL){
		swapcontext(&current->ctx, &main_ctx);
	}
	else{
		swapcontext(&current->ctx, &list->ctx);
	}
	ta_lock(mutex);


/*
	ta_unlock(mutex);
	ta_sem_wait(&cond->sem);
	//swapcontext(&cond->sem.sem_list->ctx, &main_ctx);
	//ta_lock(mutex);

*/

/*
    ta_unlock(mutex);
    struct node *head;
    head = cond->cond_list;
    // append to back
    struct node *new_node = malloc (sizeof(struct node));
    struct node *temp;
    new_node->ctx = head->ctx;
    new_node->next = NULL;
    if(cond->cond_list == NULL){
	cond->cond_list = new_node;
    }
    else{
        //cond->cond_list = cond->cond_list->next;
	temp = cond->cond_list;
	while(temp->next!=NULL){
		temp = temp->next;
	}
	temp->next = new_node;
//	head->next = new_node;
    }
    //end apend
    //list_append(&temp->ctx, &cond->cond_list); //putting it at the end of the list
    swapcontext(&head->ctx, &main_ctx);
    ta_lock(mutex);
    return;
*/


}
void ta_signal(tacond_t *cond) {
	/*ta_sem_post(&cond->sem);
    struct node *temp = cond->sem.sem_list;
    tail->next = temp;
    tail = tail->next; 
    tail->next = NULL;
    cond->sem.sem_list = cond->sem.sem_list->next;
    free(temp);
*/

    if(cond->cond_list != NULL){
	if(cond == NULL || cond->cond_list == NULL){
		return;
	}
	struct node *temp = cond->cond_list;
    	cond->cond_list = cond->cond_list->next;
	temp->next = NULL;
    	list_append(temp, list);
    	free(temp);
    }


}
