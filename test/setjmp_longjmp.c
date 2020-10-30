#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

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

void my_setjmp(struct task_ctx *ctx)
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
			:
			:);
}

void my_longjmp(struct task_ctx *ctx)
{
	asm volatile(
			"movq 0(%%rdi),%%rax\n\t"	
			"movq 16(%%rdi),%%rcx\n\t"
			"movq 24(%%rdi),%%rdx\n\t"
			"movq 48(%%rdi),%%rsp\n\t"
			"movq 56(%%rdi),%%rbp\n\t"
			"movq 64(%%rdi),%%rbx\n\t"
			"pushq %%rbx\n\t"			//push eip
			"movq 8(%%rdi),%%rbx\n\t"
			"movq 32(%%rdi),%%rdi\n\t"
			"movq 40(%%rdi),%%rsi\n\t"
			"ret\n\t"					//pop eip
			:
			:);
}

struct task_ctx ctx = {0};
jmp_buf env;

#define USR1

void test()
{
	printf("----a----\n");
#ifdef USR
	my_longjmp(&ctx);
#else
	longjmp(env,0);
#endif
}

int main(int argc,char *argv[])
{
#ifdef USR
	my_setjmp(&ctx);
#else
	setjmp(env);
#endif
	test();

	return 0;
}
