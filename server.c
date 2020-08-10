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
#include <sqlite3.h>
#include <signal.h>
#include <errno.h>

#define I 0   //用户登录
#define A 1   //增加用户
#define D 2   //删除用户
#define M 3   //修改用户
#define S 4   //检索用户
#define L 5   //登录日志
#define Q 6   //退出
#define N 16
#define R "S_login.txt"
#define BACKLOG 5

typedef struct {        //定义通讯双方的结构体
	int  type;          //超级用户  普通用户
	char account[N];    //登录帐号
	char data[256];     //登录密码
	int com;            //命令
	char name[N];       //员工姓名
	int id;             //职工号
	int salary;         //员工工资
	char address[N];    //地址
	int age;            //年龄
	int search_mode; //超级用户查询员工信息的查看方式
	int history_mode; //超级用户历史记录的查看方式
	int start_num_search; //超级用户查询员工信息起始编号
	int end_num_search; //超级用户查询员工信息结束编号
	int start_num_history; //按条件查询历史记录起始编号
	int end_num_history; //按条件查询历史记录结束编号
}MSG;
MSG msg;

void cli_info(struct sockaddr_in cin);
void do_client(int newfd,sqlite3 *db); 

void sig_child_handle(int signo)
{
	if(SIGCHLD == signo)
		waitpid(-1, NULL,  WNOHANG);
}
int main(int argc,const char *argv[])
{
	FILE *fp;
	sqlite3 *db;
	int sockfd,clientfd;
	char *errmsg;

	printf("******等待用户访问*******\n");

	if(sqlite3_open("./mydata.db",&db) !=SQLITE_OK)//打开数据库
	{
		printf("fail to open database\n");
		return -1;
	}
	else
		printf("open database successfully\n");

	//创建表
	if(sqlite3_exec(db, 
				"create table if not exists usr(account text primary key, data text,name text, id integer, salary integer,address text, age integer);",
				NULL, NULL, &errmsg) != SQLITE_OK)
	{
		printf("%s\n", errmsg);
	}
	else
		printf("Create or open usr table success.\n");

	if(sqlite3_exec(db,
				"create table if not exists record(name text ,data text,time text);",
				NULL, NULL, &errmsg) != SQLITE_OK)
	{
		printf("%s\n", errmsg);
	}
	else
		printf("Create or open record table success.\n");




	signal(SIGCHLD,sig_child_handle); //子进程结束后，会给父进程发送这个信号

	if((sockfd = socket(AF_INET,SOCK_STREAM,0)) == -1)
	{
		perror("fail to socket");
		exit(-1);
	}

	int on = 1;

	setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(int));
	struct sockaddr_in serveraddr = {
		.sin_family = AF_INET,
		.sin_addr.s_addr = INADDR_ANY,
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
		exit(1);
	}

	struct sockaddr_in cin;
	socklen_t addrlen = sizeof(cin);
	int newfd = -1;
	pid_t pid = -1;
	while(1)
	{
		bzero(&cin,sizeof(cin));
		newfd = accept(sockfd,(struct sockaddr *)&cin,&addrlen);
		if(newfd <0){
		//	perror("accept");
			continue;
		}

		if((pid=fork())<0) //创建子进程用来处理已建立链接的客户端数据收发 
		{  
			perror("fork");
			exit(1);
		}

		if(0 == pid)         //子进程
		{
			close(sockfd);       
			cli_info(cin);  //已连接客户端信息
			do_client(newfd,db); //调用收发函数
		}
		else
		{
			close(newfd);
		}
	}

}

void cli_info(struct sockaddr_in cin)
{
	char ipv4_addr[16];  //显示客户端信息
	bzero(ipv4_addr,sizeof(ipv4_addr));
	if(NULL ==inet_ntop(AF_INET,(void*)&cin.sin_addr.s_addr,ipv4_addr,sizeof(ipv4_addr))){
		perror("inet_ntop");
		exit(1);
	}
	printf("客户端(%s:%d):已连接\n",ipv4_addr,ntohs(cin.sin_port));
}

void do_client(int newfd,sqlite3 *db)
{
	while(recv(newfd, &msg,sizeof(msg),0)>0)
	{
		printf("com: %d\n",msg.com);
		switch(msg.com)
		{
		case I:
			do_login(newfd, &msg, db);
			break;
		case A:
			do_register(newfd, &msg, db);
			break;
		case D:
			do_delectuser(newfd, &msg, db);
			break;
		case M:
			do_modifyinfo(newfd, &msg, db);
			break;
		case S:
			do_selectinfo(newfd, &msg, db);
			break;
		case L:
			do_selectlogin(newfd, &msg, db);
			break;
		case Q:
			do_exit(newfd, &msg, db);
			break;
		default:
			printf("error: unknown command\n");
		}
	}
	printf("client exit\n");
	close(newfd);
	exit(0);
}
int do_register(int socketfd,MSG *msg, sqlite3 *db)
{
	char * errmsg;
	char sql[500];
	int ret =-1;
	sprintf(sql, "insert into usr values('%s', '%s',NULL,NULL,NULL,NULL,NULL);", msg->account, msg->data);
	printf("%s\n", sql);

	if(sqlite3_exec(db,sql, NULL, NULL, &errmsg) != SQLITE_OK)
	{
		printf("%s\n", errmsg);
		strcpy(msg->data, "usr name already exist.");
		ret = -1;
		send(socketfd,&ret,sizeof(ret),0);
	}
	else
	{
		ret = 1;
		printf("client  register ok!\n");
		strcpy(msg->data, "OK!");
		send(socketfd,&ret,sizeof(ret),0);
	}
		return 0;
}

int do_delectuser(int socketfd,MSG *msg, sqlite3 *db)
{
	char * errmsg;
	char sql[500];
	int ret =-1;
	sprintf(sql, "delete from user where account =\"%s\"", msg->account);
	printf("%s\n", sql);
	
}

int do_modifyinfo(int socketfd,MSG *msg, sqlite3 *db)
{

}

int do_selectinfo(int socketfd,MSG *msg, sqlite3 *db)
{


}
int do_selectlogin(int socketfd,MSG *msg, sqlite3 *db)
{

}

int do_exit(int socketfd,MSG *msg, sqlite3 *db)
{

}

int do_login(int socketfd,MSG *msg, sqlite3 *db)
{
	printf("666");
}









