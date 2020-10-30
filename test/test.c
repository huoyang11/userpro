#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

struct task_ctx src;
struct task_ctx obj;

void printf_a()
{
	printf("function start\n");
	printf("aaaaaaa\n");
	printf("function end\n");
}

void task_exit()
{
	printf("\n----task_exit----\n");
	while(1);
}

int main(int argc,char *argv[])
{
	unsigned long ptr = (unsigned long)malloc(4096);
	if(ptr == 0){
		printf("malloc error\n");
		return -1;
	}

	*((unsigned long *)(ptr + 4088)) = (unsigned long)task_exit;
	obj.esp = ptr + 4088;
	obj.ebp = obj.esp;
	obj.eip = (unsigned long)printf_a;

	Switch(&src,&obj);

	printf("------main end------\n");

	return 0;
}
