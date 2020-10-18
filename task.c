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
	rbtree user_time_set;
	queue task_end;
	struct timer_struct time_run;
	pthread_mutex_t mutex;
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

static int user_time_cmp(rbtree_node *p1,rbtree_node *p2)
{
	struct task *t1 = GET_STRUCT_START_ADDR(struct task, user_time_node, p1);
	struct task *t2 = GET_STRUCT_START_ADDR(struct task, user_time_node, p2);

	if(t1->user_time > t2->user_time) {
		return 1;
	}
	else if(t1->user_time < t2->user_time){
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
	
	while(task_context.task_end.length != 0){
		struct queue_node *p;
		queue_pop(&task_context.task_end, &p);
		struct task *pt = GET_STRUCT_START_ADDR(struct task, end_node, p);
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

int for_each(rbtree *root,rbtree_node *node)
{
    if(node == NULL || root == NULL) return -1;
    if(node == root->nil || root->root == root->nil) return 0;

    for_each(root,node->left);
	
    struct task *stu = GET_STRUCT_START_ADDR(struct task,user_time_node,node);
	
    if(stu->user_time_node.color == BLACK) {
		printf("time:%f  pid:%d BLACK\n",stu->user_time,stu->pid);
    }else{
    	printf("time:%f  pid:%d RED\n",stu->user_time,stu->pid);
    }

    for_each(root,node->right);

    return 0;
}

static void sigusr_schedul(int signum)
{
	if(signum != SIGUSR1){
		return;
	}

	if(task_context.current != NULL){
		rbtree_delete(&task_context.user_time_set, &task_context.current->user_time_node);
#if 0
		task_context.current->user_time += TASK_TIME;
		rbtree_insert(&task_context.user_time_set, &task_context.current->user_time_node);
		for_each(&task_context.user_time_set,task_context.user_time_set.root);
#else
		task_context.current->user_time += TASK_TIME;

		if(task_context.current->user_time >= 400.0){
			//printf("timeout\n");
			task_context.current->user_time = 0.0;
		}

		rbtree_insert(&task_context.user_time_set, &task_context.current->user_time_node);

		for_each(&task_context.user_time_set,task_context.user_time_set.root);
#endif
	}

	rbtree_node *rp = rbtree_min(&task_context.user_time_set, task_context.user_time_set.root);
	struct task *p = GET_STRUCT_START_ADDR(struct task, user_time_node, rp);

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
	printf("delete %d\n",current->pid);
	pthread_mutex_lock(&task_context.mutex);
	queue_push(&task_context.task_end, &current->end_node);
	pthread_mutex_unlock(&task_context.mutex);
	
	rbtree_delete(&task_context.user_time_set, &current->user_time_node);
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
	p->user_time = 0;

	task_context.current = p;

	rbtree_init(&task_context.pid_set, pid_cmp);
	rbtree_init(&task_context.user_time_set, user_time_cmp);
	queue_init(&task_context.task_end);

	rbtree_insert(&task_context.pid_set, &p->pid_node);
	rbtree_insert(&task_context.user_time_set, &p->user_time_node);

	//设置信号
	signal(SIGUSR1,sigusr_schedul);

	//启动定时器
	init_timer(&task_context.time_run, TIME_CYCLE);
	add_timer(&task_context.time_run, TASK_TIME, send_message, NULL);

	pthread_t pid;
	pthread_create(&pid,NULL,thread_callback,NULL);

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
	while(rbtree_insert(&task_context.user_time_set, &p->user_time_node) != 0){
		p->user_time += 0.0001;
	}
	cancel_signel();
	
	return 0;
}

int task_kill(int pid)
{
	shield_signel(SIGUSR1);

	rbtree_node *pnode = rbtree_find(&task_context.pid_set, find_cmp, &pid,sizeof(int));
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
	
}
