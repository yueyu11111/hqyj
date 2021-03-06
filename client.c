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

#define I 0//用户登录
#define A 1//增加用户
#define D 2//删除用户
#define M 3//修改用户信息
#define S 4//检索用户信息
#define L 5//登录日志
#define Q 6//退出
#define N 32
#define R "C_login"
typedef struct       //定义通讯双方的结构体
{
	int type;        //超级用户 普通用户
	char account[N]; //登录帐号
	char data[N];  //登录密码
	int com;         //命令名
	char name[N];    //职员姓名 
	int id;          //职工号
	int salary;      //工资
	char address[N]; //地址
	int age;         //年龄 
	int search_mode; //超级用户查询员工信息的查看方式
	int history_mode; //超级用户历史记录的查看方式
	int start_num_search; //超级用户查询员工信息起始编号
	int end_num_search; //超级用户查询员工信息结束编号
	int start_num_history; //按条件查询历史记录起始编号
	int end_num_history; //按条件查询历史记录结束编号
	char modify_which; //保存修改哪一项用户信息
}MSG;
MSG msg;

void do_add_user(int socketfd,MSG *msg,int mode);
void do_delete_user(int socketfd,MSG *msg);
void do_modify_info(int socketfd,MSG *msg);
void do_userinfo_self(int socketfd,MSG *msg,int u_type);
void do_modify_info_self(int socketfd,MSG *msg);
void do_login_history(int socketfd,MSG *msg);
void do_exit(int socketfd,MSG *msg);
void do_rootmenu();
void do_usermenu();
int  do_login();

int socketfd;
int main(int argc, const char *argv[])
{
	struct sockaddr_in server_addr;
	if(argc<3)
	{
		printf("Usage : %s <serv_ip> <serv_port>\n",argv[0]);
		exit(-1);
	}
	if((socketfd=socket(AF_INET,SOCK_STREAM,0))<0)
	{
		perror("failed to socket");
		exit(-1);
	}
	bzero(&server_addr,sizeof(server_addr));
	server_addr.sin_family=AF_INET;
	server_addr.sin_addr.s_addr=inet_addr(argv[1]);
	server_addr.sin_port = htons(atoi(argv[2]));
	if(connect(socketfd,(struct sockaddr*)&server_addr,sizeof(server_addr))<0)
	{
		perror("failed to ConnectionStatect");
		exit(-1);
	}
	do_login();
}
int do_login()
{
	int n;
	int ret=0;//用来接收超级用户 还是普通用户1 超级用户 2 普通用户
	char s; 

	while(1)
	{
		printf("**************************************\n");
		printf("     欢 迎 访 问 员 工 管 理 系 统    \n");
		printf("    1:登 录, 2:注 册 用 户, 3:退 出   \n");
		printf("**************************************\n");
		scanf("%d",&n);
		while(getchar() != '\n');
		if(n == 1)
		{
			break;
		}

		else if(n == 2)
		{
			do_add_user(socketfd,&msg,2);
		}

		else if(n == 3)
		{
			exit(0);
		}

		else
		{
			printf("输入错误，请重新输入\n");
			continue;
		}

	}
	while(1)
	{
		printf("请输入用户名:");
		if(scanf("%s",msg.account)<=0)
		{
			perror("scanf");
			exit(-1);
		}
		getchar();
		printf("请输入用户密码:");
		if(scanf("%s",msg.data)<=0)
		{
			perror("scanf");
			exit(-1);
		}
		getchar();

		msg.com = I;
		send(socketfd,&msg,sizeof(MSG),0);
		recv(socketfd,&ret,sizeof(ret),0);
		
		if(ret==49)//超级用户 字符1对应的ascii
		{
			msg.type = 1;
			do_rootmenu();
		}

		else if(ret==50)//普通用户 字符2对应的ascii
		{
			msg.type = 2;
			do_usermenu();
		}
		else
		{
			printf("用户名或密码输入错误\n");
			printf("重新登录请输入:r  退出请输入:e \n");
			scanf("%c",&s);
			switch(s)
			{
			case 'r':
				break;
			case 'e':
				exit(-1);
			}

		}

	}
}

void do_rootmenu() //管理员菜单
{
	int n;
	while(1)
	{
		printf("---------------------------------------------------------------------------------\n");
		printf("--1:添加用户 2:删除用户 3:修改用户信息 4:检索用户信息 5：查询登录日志6:退出------\n");
		printf("---------------------------------------------------------------------------------\n");
		printf("请输入命令:");
		scanf("%d",&n);
		while(getchar() != '\n');

		switch(n)
		{
		case 1:
			do_add_user(socketfd,&msg,1);
			break;

		case 2:
			do_delete_user(socketfd,&msg);
			break;

		case 3:
			do_modify_info(socketfd,&msg);
			break;

		case 4:
			while(1)
			{
				printf("----------------------------------------------------------------------------\n");
				printf("--1:查询所有用户 2：查询前10个用户 3：查询后10个用户 4：按条件查询（1-100)--\n");
				printf("----------------------------------------------------------------------------\n");
				printf("请输入命令:");
				scanf("%d",&msg.search_mode);

				if(msg.search_mode !=1&&msg.search_mode !=2&&msg.search_mode !=3&&msg.search_mode !=4)
				{
					printf("编号输入错误，请重新输入\n");
				}
				else
				{
					break;
				}
			}

			if(msg.search_mode == 4)//按条件查询，判断输入数字是否合法并跳转到查询函数
			{
				while(1)
				{
					//初始化编号
					msg.start_num_search = 0;
					msg.end_num_search = 0;

					printf("请输入起始编号:");
					scanf("%d",&msg.start_num_search);
					getchar();
					
					printf("请输入结束编号:");
					scanf("%d",&msg.end_num_search);
					getchar();

					if(msg.start_num_search > msg.end_num_search||msg.start_num_search <= 0||msg.end_num_search <= 0)
					{
						printf("编号输入错误，请重新输入\n");
						continue;
					}
					else
					{
						do_userinfo_self(socketfd,&msg,1);
						break;
					}
				}
			}

			else
			{
				do_userinfo_self(socketfd,&msg,1);
			}
			break;

		case 5:
			while(1)
			{
				printf("------------------------------------------------------------------\n");
				printf("--1:查询所有历史记录 2：查询后50条历史记录 3：按条件查询（1-100)--\n");
				printf("------------------------------------------------------------------\n");
				printf("请输入命令:");
				scanf("%d",&msg.history_mode);
				while(getchar() != '\n');

				if(msg.history_mode !=1&&msg.history_mode !=2&&msg.history_mode !=3)
				{
					printf("编号输入错误，请重新输入\n");
					continue;
				}
				else
				{
					break;
				}
			}

			if(msg.history_mode == 3)
			{
				while(1)
				{
					//初始化编号
					msg.start_num_history = 0;
					msg.end_num_history = 0;

					printf("请输入起始编号:");
					scanf("%d",&msg.start_num_history);
					printf("请输入结束编号:");
					scanf("%d",&msg.end_num_history);

					if(msg.start_num_history > msg.end_num_history||msg.start_num_history <= 0||msg.end_num_history <= 0)
					{
						printf("编号输入错误，请重新输入\n");
						continue;
					}
					else
					{
						do_login_history(socketfd,&msg);
						break;
					}
				}
			}

			else
			{
				do_login_history(socketfd,&msg);
			}
			break;

		case 6:
			do_exit(socketfd,&msg);
		}
	}
}

void do_usermenu()//用户菜单
{
	int n;
	while(1)
	{
		printf("-----------------------------------------------------------------------\n");
		printf("-------------1:检索用户信息 2:修改用户信息 3:退出----------------------\n");
		printf("-----------------------------------------------------------------------\n");
		printf("请输入命令:");                                                      
		scanf("%d",&n);
		while(getchar() != '\n');

		switch(n)
		{
		case 1:
			do_userinfo_self(socketfd,&msg,2);
			break;
		case 2:
			do_modify_info_self(socketfd,&msg);
			break;
		case 3:
			do_exit(socketfd,&msg);
		}
	}
}
// 添加用户函数,第三个参数判断是注册还是管理员添加,1代表管理员，2代表普通用户
void do_add_user(int socketfd,MSG *msg,int mode) 
{
	int ret;
	printf("请输入用户名:");
	if(scanf("%s",msg->account)<=0)
	{
		perror("scanf");
		exit(-1);
	}
	getchar();

	printf("请输入密码:");
	if(scanf("%s",msg->data)<=0)
	{
		perror("scanf");
		exit(-1);
	}
	getchar();

	msg->com  = A;
	msg->type = 2;   //2代表普通用户，客户端只能添加普通用户，所以写死

	send(socketfd,msg,sizeof(MSG),0);
	recv(socketfd,&ret,sizeof(ret),0);

	if(ret==1)//表示添加成功
	{
		printf("添加用户成功\n");
		if(mode == 1)
		{
			do_rootmenu();	
		}
		else
		{
			do_login();
		}
	}
	else
	{
		printf("添加用户失败,用户名已存在\n");
		return ;
	}

}

void do_delete_user(int socketfd,MSG *msg) //删除用户函数
{
	int ret=0;
	printf("请输入你想删除的用户名:");
	scanf("%s",msg->account);
	msg->type = 2;
	msg->com  = D;
	send(socketfd,msg,sizeof(MSG),0);
	recv(socketfd,&ret,sizeof(ret),0);
	if(ret==1)//表示删除成功
	{
		printf("删除用户成功\n");
	}
	else
	{
		printf("删除用户失败\n");
	}
	do_rootmenu();
}


void do_modify_info(int socketfd,MSG *msg) //管理员修改用户信息函数
{
	int n=0;
	int ret=0;
	while(1)
	{
		printf("请输入你想修改的用户名:");		
		scanf("%s",msg->account);
		getchar();
		msg->com = M;
		send(socketfd,msg,sizeof(MSG),0);
		recv(socketfd,&ret,sizeof(MSG),0);
		if(ret == -1)
		{
			printf("用户不存在\n");
		}
		else if(ret == 2)
			break;
	}

	while(1)
	{
		printf("*************************************************\n");
		printf("               你 想 修 改 哪 一 项              \n");
		printf("1:姓名 2：密码 3:工号 4:年龄 5:地址 6:工资 7:返回\n");
		printf("*************************************************\n");
		printf("请 选 择: ");
		scanf("%d", &n);
		while(getchar() != '\n');

		switch(n)
		{
		case 1:
			printf("请输入新姓名:");
			scanf("%s",msg->name);
			msg->modify_which = 'n';  //告诉服务器要修改姓名
			break;

		case 2:
			printf("请输入新密码:");
			scanf("%s",(msg->data));
			msg->modify_which = 'd'; //告诉服务器要修改密码
			break;

		case 3:
			printf("请输入新工号:");
			scanf("%d",&(msg->id));
			msg->modify_which = 'i'; //......			
			break;

		case 4:
			printf("请输入新年龄:");
			scanf("%d",&(msg->age));
			msg->modify_which = 'a';			
			break;

		case 5:
			printf("请输入新地址:");
			scanf("%s",(msg->address));
			msg->modify_which = 'l';
			break;

		case 6:
			printf("请输入新工资:");
			scanf("%d",&(msg->salary));
			msg->modify_which = 's';
			break;

		case 7:
			msg->modify_which = '0'; //退出，告诉服务器不做修改
			send(socketfd,msg,sizeof(MSG),0);
			msg->modify_which = '1'; //重新给modify_which赋值，防止下次修改时直接被判定为'0'
			do_rootmenu();	

		default:
			printf("输入错误，请重新输入");
			continue;			
		}

		send(socketfd,msg,sizeof(MSG),0);
		recv(socketfd,&ret,sizeof(MSG),0);

		if(ret == 1)//表示修改成功
		{
			printf("修改用户成功\n");
		}
		else
		{
			printf("修改失败\n");
		}
	}
}

//显示员工信息函数,u_type记录谁调用了这个函数，以便查询后可以正常返回函数
void do_userinfo_self(int socketfd,MSG *msg,int u_type) 
{
	msg->com=S;
	send(socketfd,msg,sizeof(MSG),0);

	if(msg->search_mode == 0) //普通用户查询自己的信息，只需要打印一遍即可
	{		
		recv(socketfd,msg,sizeof(MSG),0);
		printf("账户名:%s\n",msg->account);
		printf("账户类型：%d\n",msg->type);
		printf("职工号:%d\n",msg->id);
		printf("名字:%s\n",msg->name);
		printf("密码:%s\n",msg->data);
		printf("年龄:%d\n",msg->age);
		printf("工资:%d\n",msg->salary);
		printf("地址:%s\n",msg->address);
	}

	if(msg->search_mode != 0)//管理员查询用户信息
	{
		char str[512];
		while(1)
		{
			recv(socketfd,str,sizeof(str),0);
			printf("%s\n",str);
			if(str[0] == '\0')
				break;
		}
		do_rootmenu();
	}


	if(u_type == 1) //1代表管理员，查询后返回管理员菜单
	{
		do_rootmenu();
	}

	else //如果不是管理员调用查询函数，则返回普通用户菜单
	{
		do_usermenu();
	}
}
void do_modify_info_self(int socketfd,MSG *msg) //员工自己的修改函数
{
	int recvet=0;
	int n=0;
	int ret=0;
	msg->com = M;

	while(1)
	{
		printf("*****************************************\n");
		printf("          你 想 修 改 哪 一 项           \n");
		printf(" 1:密 码 2:姓 名 3:地 址 4:年 龄 5:返 回\n");
		printf("*****************************************\n");
		printf("请 选 择:");
		scanf("%d",&n);
		while(getchar() != '\n');

		switch(n)
		{
		case 1:
			printf("请输入新密码:");
			scanf("%s",msg->data);
			msg->modify_which = 'd';
			getchar();
			break;

		case 2:
			printf("请输入新姓名:");
			scanf("%s",msg->name);
			msg->modify_which = 'n';
			getchar();
			break;
		case 3:
			printf("请输入新地址:");
			scanf("%s",msg->address);
			msg->modify_which = 'l';
			getchar();	
			break;

		case 4:
			printf("请输入新年龄:");
			scanf("%d",&(msg->age));
			msg->modify_which = 'a';
			getchar();
			break;

		case 5:
			msg->modify_which = '0'; //退出，告诉服务器不做修改
			send(socketfd,msg,sizeof(MSG),0);
			msg->modify_which = '1'; //重新给modify_which赋值，防止下次修改时直接被判定为'0'
			do_usermenu();
		default:
			printf("输入错误，请重新输入");
			continue;	
		}

		send(socketfd,msg,sizeof(MSG),0); //由于管理员和普通用户修改用的是同一个函数，
		recv(socketfd,&ret,sizeof(MSG),0);//而管理员要send两次，所以普通用户也send两次，对齐管理员操作

		send(socketfd,msg,sizeof(MSG),0);
		recv(socketfd,&ret,sizeof(MSG),0);
		if(ret==1)//表示修改成功
		{
			printf("修改成功\n");
		}
		else
		{
			printf("修改失败\n");
		}
	}
}

void do_login_history(int socketfd,MSG *msg)  //登录日志函数
{
	char str[128]={0};
	msg->com = L;
	send(socketfd,msg,sizeof(MSG),0);
	while(1)
	{
		recv(socketfd,str,sizeof(str),0);
		printf("%s\n",str);
		if(str[0] == '\0')
			break;
	}

	do_rootmenu();
}

void do_exit(int socketfd,MSG *msg)   //退出函数
{
	msg->com = Q;
	send(socketfd,msg,sizeof(MSG),0);
	close(socketfd);
	exit(0);
}

