#ifndef __TASK_H__
#define __TASK_H__

#include "rbtree_node.h"
#include "queue.h"

#define TASK_RUN   1
#define TASK_END   2

struct task_ctx
{
	unsigned long eax;
	unsigned long ebx;
	unsigned long ecx;
	unsigned long edx;
	unsigned long edi;
	unsigned long esi;
	unsigned long esp;
	unsigned long ebp;
	unsigned long eip;
};

struct task
{
	int state;
	int pid;
	void *start_addr;
	double user_time;
	rbtree_node user_time_node;
	rbtree_node pid_node;
	queue_node end_node;
	struct task_ctx ctx;
	void (*start_routine)(void *);
	void *arg;
};

int task_init();
int task_switch(struct task *src,struct task *obj);
int task_create(void (*start_routine)(void *),void *arg);
void task_exit();
int task_kill(int pid);
int task_uninit();

#endif
