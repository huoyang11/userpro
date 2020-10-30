#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>

#include "task.h"
#include "timer.h"

static struct 
{
	struct task *current;
	rbtree pid_set;
	queue task_end;
	queue task_ready;
	struct timer_struct time_run;
	pthread_mutex_t mutex;
	pthread_t pid;
}task_context;

static int find_cmp(void *arg,int size)
{
	rbtree_node *pnode = (rbtree_node *)*((unsigned long *)arg);
	struct task *p = GET_STRUCT_START_ADDR(struct task, pid_node, pnode);

	int pid = *((int *)(arg + sizeof(pnode)));

	if(p->pid > pid){
		return 1;
	}else if(p->pid < pid){
		return -1;
	}

	return 0;
}

static int pid_cmp(rbtree_node *p1,rbtree_node *p2)
{
	struct task *t1 = GET_STRUCT_START_ADDR(struct task, pid_node, p1);
	struct task *t2 = GET_STRUCT_START_ADDR(struct task, pid_node, p2);

	if(t1->pid > t2->pid) {
		return 1;
	}
	else if(t1->pid < t2->pid){
		return -1;
	}

	return 0;
}

static void Switch(struct task_ctx *src,struct task_ctx *obj)
{
	asm volatile(
	"movq %%rax,0(%%rdi)\n\t"
	"movq %%rbx,8(%%rdi)\n\t"
	"movq %%rcx,16(%%rdi)\n\t"
	"movq %%rdx,24(%%rdi)\n\t"
	"movq %%rdi,32(%%rdi)\n\t"
	"movq %%rsi,40(%%rdi)\n\t"
	"movq %%rbp,%%rbx\n\t"
	"add $16,%%rbx\n\t"
	"movq %%rbx,48(%%rdi)\n\t"  //save esp
	"movq 0(%%rbp),%%rbx\n\t"
	"movq %%rbx,56(%%rdi)\n\t"	//save ebp
	"movq 8(%%rbp),%%rbx\n\t"
	"movq %%rbx,64(%%rdi)\n\t"  //save eip
	"movq 0(%%rsi),%%rax\n\t"	
	"movq 16(%%rsi),%%rcx\n\t"
	"movq 24(%%rsi),%%rdx\n\t"
	"movq 48(%%rsi),%%rsp\n\t"
	"movq 56(%%rsi),%%rbp\n\t"
	"movq 64(%%rsi),%%rbx\n\t"
	"pushq %%rbx\n\t"			//push eip
	"movq 8(%%rsi),%%rbx\n\t"
	"movq 32(%%rsi),%%rdi\n\t"
	"movq 40(%%rsi),%%rsi\n\t"
	"ret\n\t"					//pop eip
	:
	:);
}

static void cancel_signel()
{
	sigset_t set;
	sigemptyset(&set);
	pthread_sigmask(SIG_SETMASK, &set, NULL);
}

static void shield_signel(int sig)
{
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, sig);
	pthread_sigmask(SIG_SETMASK, &set, NULL);
}

static void send_message(void *arg)
{
	kill(getpid(),SIGUSR1);

	pthread_mutex_lock(&task_context.mutex);
	//回收协程空间
	while(!queue_isempty(&task_context.task_end)){
		struct queue_node *p;
		queue_pop(&task_context.task_end, &p);
		struct task *pt = GET_STRUCT_START_ADDR(struct task, end_node, p);
		//printf("free pid:%d\n",pt->pid);
		free(pt->start_addr);
		free(pt);
	}
	
	pthread_mutex_unlock(&task_context.mutex);
}

static void *thread_callback(void *arg)
{
	shield_signel(SIGUSR1);

	run_timer(&task_context.time_run);
}

static void sigusr_schedul(int signum)
{
	if(signum != SIGUSR1){
		return;
	}

	if(task_context.current != NULL){
		queue_push(&task_context.task_ready, &task_context.current->ready_node);
	}

	struct queue_node *rp = NULL;
	queue_pop(&task_context.task_ready, &rp);
	struct task *p = GET_STRUCT_START_ADDR(struct task, ready_node, rp);

	if(p == task_context.current){
		cancel_signel();
		return;
	}

	struct task *current = task_context.current;
	task_context.current = p;
	
	cancel_signel();

	if(current == NULL){
		struct task tem;
		task_switch(&tem, p);
	}else{
		task_switch(current, p);
	}
}

int task_switch(struct task *src,struct task *obj)
{
	if(src == NULL || obj == NULL){
		return -1;
	}

	Switch(&(src->ctx), &(obj->ctx));

	return 0;
}

void task_exit()
{
	shield_signel(SIGUSR1);
	struct task *current = task_context.current;
	current->state = TASK_END;
	
	pthread_mutex_lock(&task_context.mutex);
	queue_push(&task_context.task_end, &current->end_node);
	pthread_mutex_unlock(&task_context.mutex);
	
	rbtree_delete(&task_context.pid_set, &current->pid_node);
	task_context.current = NULL;
	cancel_signel();

	while(1);
}


int task_init()
{
	pthread_mutex_init(&task_context.mutex,NULL);

	struct task *p = (struct task *)malloc(sizeof(struct task));
	if(p == NULL){
		return -1;
	}

	p->pid = 0;
	p->state = TASK_RUN;

	task_context.current = p;

	rbtree_init(&task_context.pid_set, pid_cmp);
	queue_init(&task_context.task_ready);
	queue_init(&task_context.task_end);

	rbtree_insert(&task_context.pid_set, &p->pid_node);

	//设置信号
	signal(SIGUSR1,sigusr_schedul);

	//启动定时器
	init_timer(&task_context.time_run, TIME_CYCLE);
	add_timer(&task_context.time_run, TASK_TIME, send_message, NULL);

	pthread_create(&task_context.pid,NULL,thread_callback,NULL);

	return 0;
}

int task_create(void (*start_routine)(void *),void *arg)
{
	if(start_routine == NULL){
		return -1;
	}

	if(task_context.pid_set.cmp != pid_cmp){
		task_init();
	}

	static int index = 1;
	struct task *p = (struct task *)calloc(1,sizeof(struct task));
	if(p == NULL){
		return -2;
	}

	p->pid = index++;
	p->start_routine = start_routine;
	p->arg = arg;
	p->state = TASK_RUN;

	p->ctx.eip = (unsigned long)start_routine;
	p->ctx.edi = (unsigned long)arg;

	p->start_addr = calloc(1,10240);
	if(p->start_addr == 0){
		return -3;
	}
	
	p->ctx.esp = (unsigned long)p->start_addr + 10232;
	*((unsigned long *)p->ctx.esp) = (unsigned long)task_exit;
	p->ctx.ebp = p->ctx.esp;

	shield_signel(SIGUSR1);
	
	rbtree_insert(&task_context.pid_set, &p->pid_node);
	queue_push(&task_context.task_ready, &p->ready_node);
	
	cancel_signel();
	
	return 0;
}

int task_kill(int pid)
{
	shield_signel(SIGUSR1);

	rbtree_node *pnode = rbtree_find(&task_context.pid_set, find_cmp, &pid,sizeof(int));
	if(pnode == NULL){
		return -1;
	}
	struct task *p = GET_STRUCT_START_ADDR(struct task, pid_node, pnode);
	
	p->ctx.eip = (unsigned long)task_exit;
	p->ctx.esp = p->ctx.ebp;

	struct task *current = task_context.current;

	cancel_signel();
	
	if(current == p){
		while(1);
	}

	return 0;
}

int task_uninit()
{
	shield_signel(SIGUSR1);
	stop_timer(&task_context.time_run);
	pthread_join(task_context.pid,NULL);
	uninit_timer(&task_context.time_run);
	
	rbtree_destroy(&task_context.pid_set);

	int isfree = 0;
	if(task_context.current != NULL){
		struct task *current = task_context.current;
		printf("pid:%d\n",current->pid);
		if(current->pid != 0){
			free(current->start_addr);
			free(current);
		}else{
			isfree = 1;
			free(current);
		}	
	}

	while(!queue_isempty(&task_context.task_end)){
		struct queue_node *p;
		queue_pop(&task_context.task_end, &p);
		struct task *pt = GET_STRUCT_START_ADDR(struct task, end_node, p);
		printf("pid:%d\n",pt->pid);
		if(pt->pid != 0){
			free(pt->start_addr);
			free(pt);
		}else{
			if(!isfree){
				isfree = 1;
				free(pt);
			}
		}		
	}

	while(!queue_isempty(&task_context.task_ready)){
		struct queue_node *p;
		queue_pop(&task_context.task_ready, &p);
		struct task *pt = GET_STRUCT_START_ADDR(struct task, ready_node, p);
		printf("pid:%d\n",pt->pid);
		if(pt->pid != 0){
			free(pt->start_addr);
			free(pt);
		}else{
			if(!isfree){
				isfree = 1;
				free(pt);
			}
		}
	}

	queue_uninit(&task_context.task_ready);
	queue_uninit(&task_context.task_end);

	cancel_signel();

	return 0;
}
