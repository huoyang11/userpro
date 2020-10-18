#ifndef __QUEUE_H__
#define __QUEUE_H__

#include "config.h"

typedef struct queue_node
{
	struct queue_node *prev;
	struct queue_node *next;
}queue_node;

typedef struct queue
{
	int length;
	struct queue_node *head;
	struct queue_node *last;
}queue;

int queue_init(struct queue *q);
int queue_push(struct queue *q ,struct queue_node *n);
int queue_pop(struct queue *q ,struct queue_node **n);
int queue_uninit(struct queue *q);

#endif