#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "task.h"

int init_listen(short port)
{
	int ser_fd = socket(AF_INET,SOCK_STREAM,0);
	if(ser_fd == -1) 
	{	
		perror("create socket error");
		return -1;
	}	

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = 0;

	if(bind(ser_fd,(struct sockaddr *)&addr,sizeof(addr)) < 0)
	{	
		perror("bind ip and port error");
		return -1;
	}	

	if(listen(ser_fd,20) < 0)
	{	
		perror("listen error");
		return -1;
	}

	return ser_fd;
}

void task_callback(void *arg)
{
	int cfd = *((int *)arg);
	char buf[512] = {0};
	
	while(1){
		int len = recv(cfd,buf,sizeof(buf),0);
		printf("recv>%d:",cfd);
		fwrite(buf,1,len,stdout);
	}
}

int main(int argc,char *argv[])
{

	int sfd = init_listen(9998);
	if(sfd < 0){
		perror("init_listen error");
		return -1;
	}
	
	struct sockaddr_in cli_addr;
	socklen_t addrlen = sizeof(cli_addr);

	while(1){
		int cfd = accept(sfd,(struct sockaddr *)&cli_addr,&addrlen);
		if(cfd != -1){
			printf("accept %d\n",cfd);
		}
		task_create(task_callback,&cfd);
	}
	return 0;
}
