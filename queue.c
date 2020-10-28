#include <string.h>
#include <stdlib.h>
#include "queue.h"

int queue_init(struct queue *q)
{
	if(q == NULL){
		return -1;
	}

	memset(q,0,sizeof(struct queue));
	
	return 0;
}

int queue_push(struct queue *q ,struct queue_node *n)
{
	if(q == NULL || n == NULL){
		return -1;
	}

	memset(n,0,sizeof(struct queue_node));
	if(q->length == 0){
		q->last = n;
		q->head = n;
	}else{		
		n->next = q->head;
		q->head->prev = n;
		q->head = n;
	}
	
	q->length++;
	
	return 0;
}

int queue_isempty(struct queue *q)
{
	if(q == NULL){
		return -1;
	}

	if(q->length == 0){
		return 1;
	}

	return 0;
}

int queue_pop(struct queue *q ,struct queue_node **n)
{
	if(q == NULL){
		return -1;
	}

	if(q->length == 0){
		if(n != NULL){
			*n = NULL;
		}
		return -2;
	}

	if(n != NULL){
		*n = q->last;
	}
	
	q->last = q->last->prev;
	q->length--;

	return 0;
}

int queue_uninit(struct queue *q)
{
	if(q == NULL){
		return -1;
	}

	memset(q,0,sizeof(struct queue));

	return 0;
}
