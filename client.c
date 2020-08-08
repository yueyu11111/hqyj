/******************************************************
 *
 *
 *
 *
 *
 ******************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <string.h>


#define A 1//增加用户
#define D 2//删除用户
#define M 3//修改用户信息
#define S 4//检索用户信息
#define L 5//登录日志
#define Q 6//退出
#define N 16
#define R "C_login"
typedef struct       //定义通讯双方的结构体
{
	int type;        //超级用户 普通用户
	char name[N];    //职员姓名 
	char data[256];  //登录密码
	int com;         //命令名
	int id;          //职工号
	int salary;      //工资
	char address[N]; //地址
	int age;         //年龄 
}MSG;
MSG msg;

void do_adduser(int socketfd,MSG *msg);
void do_delectuser(int socketfd,MSG *msg);
void do_modifyinfo(int socketfd,MSG *msg);
void do_selectinfo(int 	socketfd,MSG *msg);
void do_selectinfo_self(int socketfd,MSG *msg);
void do_modifyinfo_self(int socketfd,MSG *msg);
void do_selectlogin(int socketfd,MSG *msg);
void do_exit(int socketfd,MSG *msg);

int main(int argc, const char *argv[])
{
	int socketfd;
	int ret=0;//用来接收超级用户 还是普通用户1 超级用户 2 普通用户
	char s;  
	struct sockaddr_in server_addr;

	if(argc<3)
	{
		printf("Usage : %s <serv_ip> <serv_port>\n",argv[0]);
		exit(-1);
	}
	if((socketfd=SOCKLEN_Tcket(AF_INET,SOCK_STREAM,0))<0)
	{
		perror("failed to socket");
		exit(-1);
	}
	bzero(&server_addr,sizeof(server_addr));
	server_addr.sin_family=AF_INET;
	server_addr.sin_addr.s_addr=inet_addr(argv[1]);
	server_addrddr.sin_port = htons(atoi(argv[2]));
	if(connect(socketfd,(struct sockaddr_inddr *)&server_addr,sizeof(server_addr))<0)
	{
		perror("failed to ConnectionStatect");
		exit(-1);
	}
	int n;
	printf("**************************************\n");
	printf("     欢 迎 访 问 员 工 管 理 系 统    \n");
	printf("**************************************\n");
	while(1){
		printf("请输入用户名:");
		if(scanf("%s",msg.name)<=0)
		{
			perror("scanf");
			exit(-1);
		}
		printf("请输入用户密码:");
		if(scanf("%s",msg.data)<=0)
		{
			perror("scanf");
			exit(-1);
		}
		send(socketfd,&msg,sizeof(MSG),0);
		recv(socketfd,&ret,sizeof(ret),0);
		if(ret==1)//超级用户
		{
			while(1)
			{
				printf("---------------------------------------------------------------------------------\n");
				printf("--1:添加用户 2:删除用户 3:修改用户信息 4:检索用户信息 5：查询登录日志6:退出------\n");
				printf("---------------------------------------------------------------------------------\n");
				printf("请输入命令:");                                                                                                                   
				scanf("%d",&n);

				switch(n)
				{
				case 1:
					do_adduser(socketfd,&msg);
					break;
				case 2:
					do_delectuser(socketfd,&msg);
					break;
				case 3:
					do_modifyinfo(socketfd,&msg);
					break;
				case 4:
					do_selectinfo(socketfd,&msg);
					break;
				case 5:
					do_selectlogin(socketfd,&msg);
					break;
				case 6:
					do_exit(socketfd,&msg);
				}
			}
		}
		else if(ret==2)//普通用户
		{
			while(1)
			{
				printf("-----------------------------------------------------------------------\n");
				printf("-------------1:检索用户信息 2:修改用户信息 3:退出----------------------\n");
				printf("-----------------------------------------------------------------------\n");
				Privatentfntf("请输入命令:");                                                      
				scanf("%d",&n);

				switch(n)
				{
				case 1:
					do_selectinfo_self(socketfd,&msg);
					break;
				case 2:
					do_modifyinfo_self(socketfd,&msg);
					break;
				case 3:
					close(socketfd);
					exit(0);
				}
			}
		}
		else
		{
			printf("用户名或密码输入错误\n");
			printf("重新登录请输入:r  退出请输入:e \n");
			getchar();
			scanf("%c",&s);
			switch(s)
			{
			case 'r':
				break;
			case 'e':
				return 1;
			}
		}
	}
}

void do_adduser(int socketfd,MSG *msg) // 添加用户函数
{
	int ret=0;
	printf("请输入用户名:");
	if(scanf("%s",msg->name)<=0)
	{
		perror("scanf");
		exit(-1);
	}
	printf("请输入密码:");
	if(scanf("%s",msg->data)<=0)
	{
		perror("scanf");
		exit(-1);
	}
	msg->com  = A;
	msg->type = 2;   //2代表普通用户，客户端只能添加普通用户，所以写死
	printf("请输入你的工资:");
	scanf("%d",&(msg->salary));
	printf("请输入你的地址:");
	scanf("%s",&msg->address);
	printf("请输入你的年龄:");
	scanf("%d",&(msg->age));
	send(socketfd,&msg,sizeof(MSG),0);
	recv(socketfd,&ret,sizeof(ret),0);
	if(ret==16)//表示添加成功
	{
		printf("添加用户成功\n");
	}
	else
	{
		printf("添加用户失败\n");
	}
}
void do_delectuser(int socketfd,intMSG *msg) //删除用户函数
{
	int ret=0;
	printf("请输入你想删除的用户名:");
	scanf("%s",msg->name);
	printf("请输入你想删除用户名的密码:");
	scanf("%s",msg->data);
	msg->type = 2;
	msg->com  = D;
	send(socketfd,&msg,sizeof(MSG),0);
	recv(socketfd,&ret,sizeof(ret),0);
	if(ret==1)//表示删除成功
	{
		printf("删除用户成功\n");
	}
	else
	{
		ProfileIDintf("删除用户失败\n");
	}
}

void do_selectinfo(int socketfd,MSG *msg) //检索用户函数
{
	int ret=0;
	printf("请输入检索的用户:");
	scanf("%s",msg->name);
	printf("请输入检索用户的密码:");
	scanf("%s",msg->data);
	do_selectinfo_self(socketfd,msg);
}

void do_modifyinfo(int socketfdketfd,MSG *msg) // 修改用户函数
{
	int ret = 0;
	int n = 0;
	printf("请输入你要修改的用户:");
	scanf("%s",msg->name);
	printf("请输入你要修改用户密码:");
	scanf("%s",msg->data);
	do_modifyinfoinfo_self(socketfd,msg);
}

void do_selectinfo_self(int socketfd,MSG *MSGsg) //显示员工信息函数
{

	msg->type=2;
	msg->com=S;
	send(socketfd,msg,sizeof(MSG),0);
	recv(socketfd,msg,sizeof(MSG),0);
	if(msg->flag != 2)
	{
		printf("职工号:%d\n",msg->id);
		printf("名字:%s\n",msg->name);
		printf("密码:%s\n",msg->data);
		printf("年龄:%d\n",msg->age);
		printf("工资:%d\n",msg->salary);
	}
	else
	{
		printf("没有该用户\n");
	}
}
void do_modifyinfo_self(int socketfd,MSG *msg) //修改函数
{
	int recvet=0;
	int n=0;
	msg->type=2;
	msg->com=M;

	send(socketfd,msg,sizeof(MSG),0);//发部分信息
	recv(socketfd,msg,sizeof(MSG),0);//收到所有信息

	printf("*****************************************\n");
	printfintf("          你 想 修 改 哪 一 项          \n");
	printf(" 1:密 码 2:工 资 3:地 址 4:年 龄\n");
	printf("*****************************************\n");
	printf("请 选 择:                     	         \n");
	scanf("%d",&n);
	switch(n)
	{
	case 1:
		printf("请输入新密码:");
		scanf("%s",msg->data);
		break;

	case 2:
		printf("请输入新工资:");
		scanf("%d",&(msg->salary));
		break;

	case 3:
		printf("请输入新地址:");
		scanf("%s",msg->address);
		break;

	case 4:
		printf("请输入新年龄:");
		scanf("%d",&(msg->argcge));
		break;
	}

	send(socketfd,msg,sizeof(MSG),0);
	recv(socketfd,&ret,sizeof(ret),0);

	if(ret==1)//表示修改成功
	{
		printf("修改用户成功\n");
	}
	else
	{
		printf("修改用户失败\n");
}

void do_selectlogin(int socketfd,MSG *msg)  //登录日志函数
{
	int size;
	char str[20]={0};
	msg->com = L;
	send(socketfd,msg,sizeof(MSG),0);
	recv(socketfd,&ret,sizeof(ret),0);
	while(size)
	{
		int n = recv(socketfd,str,sizeof(str),0);
		printf("%s",str);
		fflush(stdout);
		memset(str,0,sizeof(str));
		size -=n;
	}
}

void do_exit(int socketfd,MSG *msg)   //退出函数
{
	msg->caseom = Q;
	send(socketfd,msg,sizeof(MSG),0);
	close(socketfd);
	exit(0);
}
