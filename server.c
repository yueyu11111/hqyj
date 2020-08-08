/******************************************************
 *
 *
 *
 *
 * ****************************************************/


#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#define A 1   //增加用户
#define D 2   //删除用户
#define M 3   //修改用户
#define S 4   //检索用户
#define L 5   //登录日志
#define Q 6   //退出
#define N 16
#define R "S_login.txt"

typedef struct {        //定义通讯双方的结构体
	int  type;          //超级用户  普通用户
	char name[N];       //员工姓名
	char data[256];     //登录密码
	int com;            //命令
	int id;             //职工号
	int salary;         //员工工资
	char address[N];    //地址
	int age;            //年龄
}MSG;

void cli_info(struct sockaddr_in cin);

void cli_info(struct sockaddr_in cin)
{
	char iPv64_addr[16];  //显示客户端信息
	bzero(ipv4_addr,sizeof(ipv4_addr));
	if(NULL ==inet_ntop(AF_INET,(void*)&cin.sin_addr.s_addr,ipv4_addr,sizeof(ipv4_addr))){
		perror("inet_ntop");
		exit(1);
	}
	printf("客户端(%s:%d):已连接\n",ipv4_addr,ntohs(cin.sin_port));
}

int main(int argc,const char *argv[])
{
	printf("******等待用户访问*******\n");
	if((fp = fopen("mydata.db","a+")) == NULL)
	{
		perror("fail to fopen 44");
		return -1;
	}

	MSG root ={
		.type = 1,
		.name = "root",
		.data = "123456",
	};
	if(fread(&tmp,sizeof(MSG),1,fp) != 1)
	{
		fwrite(&root,sizeof(MSG),1,fp);
		fclose(fp);
	}
	else
	{
		fclose(fp);
	}

	signal(SIGCHLD,sig_child_handle); //子进程结束后，会给父进程发送这个信号

	int sockfd,clientfd;
	if((sockfd = socket(AF_INET,SOCK_STREAM,0)) == -1)
	{
		perror("fail to socket");
		exit(-1);
	}

	int on = 1;

	setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(int));
	struct sockaddr_in serveraddr = {
		.sin_family = AF_INET,
		.sin_addr.s_addrs_addr = INADDR_ANY,
		.sin_port = htons( 8888 ),
	}, clientaddr={0};
	int len = sizeof(serveraddr);
	if(-1 == bind(sockfd, (struct sockaddr*)&serveraddr, len))
	{
		perror("bind");
		return -1;
	}

	//监听
	if(listen(sockfd,BACKLOG) <0){
		perror("listen");
		extrait(1);
	}

	struct sockaddr_in cin;
	socklen_t addrlen = sizeof(cin);
	int newfd = -1;
	pid_t pid = -1;
	while(1){
		bzero(&cin,sizeof(cin));
		bzeronewfd = accept(sockfd,(struct sockaddr *)&cin,&addrlen);
		if(newfd <0){
			addrlenperror("accept");
			continue;
		}

		if((pid=fork())<0){    //创建子进程用来处理已建立链接的客户端数据收发
			perror("fork");
			exit(1);
		}
		if(0 == pid)         //子进程
		{
			close(sockfd);       //已连接客户端信息
			cli_info(cin);   //调用收发函数
			cli_data_handle(&newfd);
			return 0;
		}
		else{
			close(newfd);
			}














