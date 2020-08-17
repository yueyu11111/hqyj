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
#define N 32
#define R "S_login.txt"
#define BACKLOG 5

typedef struct {        //定义通讯双方的结构体
	int  type;          //超级用户  普通用户
	char account[N];    //登录帐号
	char data[N];     //登录密码
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
	char modify_which; //保存修改哪一项用户信息
}MSG;
MSG msg;

int userinfo_callback(void* arg,int f_num,char** f_value,char** f_name);
int history_callback(void* arg,int f_num,char** f_value,char** f_name);

void cli_info(struct sockaddr_in cin);
void do_client(int newfd,sqlite3 *db); 
int do_register(int socketfd,MSG *msg, sqlite3 *db);
int do_delete_user(int socketfd,MSG *msg, sqlite3 *db);
int do_modify_info(int socketfd,MSG *msg, sqlite3 *db);
int do_userinfo(int socketfd,MSG *msg, sqlite3 *db);
int do_login_history(int socketfd,MSG *msg, sqlite3 *db);
int do_exit(MSG *msg, sqlite3 *db);
int do_login(int socketfd,MSG *msg, sqlite3 *db);




void sig_child_handle(int signo)
{
	if(SIGCHLD == signo)
		waitpid(-1, NULL,  WNOHANG);
}


char exittime[128]={0};//记录退出时间
char logintime[128]={0};//记录登录时间
char loginaccount[N]={0}; //记录登录时的账号，以便写入登录日志


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
		"create table if not exists usr(account text primary key, data text,type integer,name text, id integer, salary integer,address text, age integer);",
		NULL, NULL, &errmsg) != SQLITE_OK)
	{
		printf("%s\n", errmsg);
	}
	else
		printf("Create or open usr table success.\n");

	if(sqlite3_exec(db,
		"create table if not exists record(id INTEGER primary key AUTOINCREMENT,account text,logintime text,exittime text);",
		NULL, NULL, &errmsg) != SQLITE_OK)
	{
		printf("%s\n", errmsg);
	}
	else
		printf("Create or open record table success.\n");

	//添加root用户
	if(sqlite3_exec(db,"insert into usr(account,data,type)select 'root','123',1 WHERE NOT EXISTS(SELECT 1 FROM usr WHERE account = 'root' AND data = '123' and type = 1);",
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

void do_client(int newfd,sqlite3 *db) //根据客户端的命令跳转到相应的函数
{

	while(recv(newfd, &msg,sizeof(MSG),0)>0)
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
				do_delete_user(newfd, &msg, db);
				break;
			case M:
				do_modify_info(newfd, &msg, db);
				break;
			case S:
				do_userinfo(newfd, &msg, db);
				break;
			case L:
				do_login_history(newfd, &msg, db);
				break;
			case Q:
				do_exit(&msg, db);
				close(newfd);
				exit(0);
				break;
			default:
				printf("error: unknown command\n");
		}
	}
	printf("client exit\n");
	if(*(msg.account) !='\0')//强制退出也会记录退出时间,但是没有登录就强制退出不会记录退出时间
	{
		do_exit(&msg, db);
	}
	close(newfd);
	exit(0);
}
int do_register(int socketfd,MSG *msg, sqlite3 *db)
{
	char * errmsg;
	char sql[500];
	int ret =-1;
	sprintf(sql, "insert into usr values('%s', '%s',2,NULL,NULL,NULL,NULL,NULL);", msg->account, msg->data);
	printf("%s\n", sql);

	if(sqlite3_exec(db,sql, NULL, NULL, &errmsg) != SQLITE_OK)
	{
		printf("%s\n", errmsg);
		ret = -1;
		send(socketfd,&ret,sizeof(ret),0);
	}
	else
	{
		ret = 1;
		printf("client  register ok!\n");
		send(socketfd,&ret,sizeof(ret),0);
	}
	return 0;
}

int do_delete_user(int socketfd,MSG *msg, sqlite3 *db)
{
	char * errmsg;
	char sql[500];
	int ret =-1;
	sprintf(sql, "delete from usr where account =\"%s\";", msg->account);
	printf("%s\n", sql);
	if(sqlite3_exec(db,sql, NULL, NULL, &errmsg) != SQLITE_OK)
	{
		printf("%s\n", errmsg);
		ret = -1;
		send(socketfd,&ret,sizeof(ret),0);
	}
	else
	{
		printf("delete success");
		ret = 1;
		send(socketfd,&ret,sizeof(ret),0);
	}	
	return 0;
}

int do_modify_info(int socketfd,MSG *msg, sqlite3 *db) //修改用户信息
{
	char * errmsg;
	char sql[500];
	int nrow;
	int ncloumn;
	char **resultp;
	int ret = -1;
	int ret1 = 1,ret2 = 1,ret3 = 1,ret4 = 1,ret5 = 1,ret6 = 1;
	if(msg->modify_which == '0')//用户什么都没修改就返回时，退出修改函数
		return 0;
	sprintf(sql,"select * from usr where account = '%s';",msg->account);
	sqlite3_get_table(db,sql,&resultp, &nrow, &ncloumn, &errmsg); //管理员修改时，首先验证用户是否存在(nrow == 0)
 	if(nrow == 0) //因为管理员可以修改任意用户信息，所以管理员修改用户时，要检索是否存在该用户，如果不存在返回用户不存在
 	{
 		printf("no user\n");
 		ret = -1;
 		send(socketfd,&ret,sizeof(ret),0);
 	}
 	else
 	{
 		ret = 2;
 		send(socketfd,&ret,sizeof(ret),0);
 		while(1)
 		{
 			recv(socketfd,msg,sizeof(MSG),0);
 			if(msg->modify_which == '0')// 用户修改了内容后要返回时，退出修改函数
 				return 0;
 			if(msg->modify_which == 'n')
 			{
 				sprintf(sql,"update usr set name='%s' where account = '%s';",msg->name,msg->account);
 				if(sqlite3_exec(db,sql, NULL, NULL, &errmsg) != SQLITE_OK)
 				{
 					printf("%s\n", errmsg);
 					ret1 = -1;
 				}
 				else
 				{
 					printf("modify name success\n");
 				}
 			}

 			if(msg->modify_which == 'd')
 			{
 				sprintf(sql,"update usr set data='%s' where account = '%s';",msg->data,msg->account);
 				if(sqlite3_exec(db,sql, NULL, NULL, &errmsg) != SQLITE_OK)
 				{
 					printf("%s\n", errmsg);
 					ret2 = -1;
 				}
 				else
 				{
 					printf("modify data success\n");
 				}
 			}

 			if(msg->modify_which == 'i')
 			{
 				sprintf(sql,"update usr set id=%d where account = '%s';",msg->id,msg->account);
 				if(sqlite3_exec(db,sql, NULL, NULL, &errmsg) != SQLITE_OK)
 				{
 					printf("%s\n", errmsg);
 					ret3 = -1;
 				}
 				else
 				{
 					printf("modify id success\n");
 				}
 			}

 			if(msg->modify_which == 'a')
 			{
 				sprintf(sql,"update usr set age=%d where account = '%s';",msg->age,msg->account);
 				if(sqlite3_exec(db,sql, NULL, NULL, &errmsg) != SQLITE_OK)
 				{
 					printf("%s\n", errmsg);
 					ret4 = -1;
 				}
 				else
 				{
 					printf("modify age success\n");
 				}
 			}

 			if(msg->modify_which == 'l')
 			{
 				sprintf(sql,"update usr set address='%s' where account = '%s';",msg->address,msg->account);
 				if(sqlite3_exec(db,sql, NULL, NULL, &errmsg) != SQLITE_OK)
 				{
 					printf("%s\n", errmsg);
 					ret5 = -1;
 				}
 				else
 				{
 					printf("modify address success\n");
 				}
 			}

 			if(msg->modify_which == 's')
 			{
 				sprintf(sql,"update usr set salary=%d where account = '%s';",msg->salary,msg->account);
 				if(sqlite3_exec(db,sql, NULL, NULL, &errmsg) != SQLITE_OK)
 				{
 					printf("%s\n", errmsg);
 					ret6 = -1;
 				}
 				else
 				{
 					printf("modify salary success\n");
 				}
 			}

			if(ret1==1 && ret2==1 && ret3==1 && ret4==1 && ret5==1 && ret6==1) //修改成功返回1
			{
				ret = 1;
				send(socketfd,&ret,sizeof(int),0);
			}
			else
			{
				printf("modify failed"); //如果有未修改成功的资料，则返回修改失败
				ret = -2;
				send(socketfd,&ret,sizeof(int),0);
			}
		}
	}
}

int do_userinfo(int socketfd,MSG *msg, sqlite3 *db)
{
	char sql[128] = {0};
	char *errmsg;
	int nrow;
	int ncloumn;
	char **resultp;
	char end = '\0';

	if(msg->search_mode == 0) //这里应该用回调函数的，一开始写的没有注意，虽然比较复杂，但是功能可以实现
	{
		sprintf(sql,"select id from usr where account = '%s';",msg->account);
		sqlite3_get_table(db,sql,&resultp, &nrow, &ncloumn, &errmsg);
		if(resultp[1] != 0)//防止给atoi传空值引起程序崩溃
		{
			msg->id = atoi(resultp[1]);
		}

		printf("id ok\n");
		sprintf(sql,"select name from usr where account = '%s';",msg->account);
		sqlite3_get_table(db,sql,&resultp, &nrow, &ncloumn, &errmsg);
		if(resultp[1] != 0)
		{
			strcpy(msg->name,resultp[1]);
		}
		printf("name ok\n");

		sprintf(sql,"select age from usr where account = '%s';",msg->account);
		sqlite3_get_table(db,sql,&resultp, &nrow, &ncloumn, &errmsg);
		if(resultp[1] != 0)
		{
			msg->age = atoi(resultp[1]);
		}
		printf("age ok\n");

		sprintf(sql,"select salary from usr where account = '%s';",msg->account);
		sqlite3_get_table(db,sql,&resultp, &nrow, &ncloumn, &errmsg);
		if(resultp[1] != 0)
		{
			printf("atoi salary\n");
			msg->salary = atoi(resultp[1]);
		}
		printf("salary ok\n");

		sprintf(sql,"select address from usr where account = '%s';",msg->account);
		sqlite3_get_table(db,sql,&resultp, &nrow, &ncloumn, &errmsg);
		if(resultp[1] != 0)
		{
			strcpy(msg->address, resultp[1]);
		}

		printf("account = %s",msg->account);
		send(socketfd,msg,sizeof(MSG),0);

		printf("search success\n");
	}

	else if(msg->search_mode == 1) //管理员查询所有用户信息
	{
		if(sqlite3_exec(db, "select * from usr order by account;", userinfo_callback,(void *)&socketfd, &errmsg)!= SQLITE_OK)
		{
			printf("%s\n", errmsg);
			send(socketfd,errmsg,sizeof(strlen(errmsg)),0);
			send(socketfd,&end,sizeof(end),0);
		}
		else
		{
			printf("Query all_users done.\n");
			send(socketfd,&end,sizeof(end),0);
		}
	}

	else if(msg->search_mode == 2) // 管理员查询前10个用户信息
	{
		if(sqlite3_exec(db, "select * from usr order by account limit 10;", userinfo_callback,(void *)&socketfd, &errmsg)!= SQLITE_OK)
		{
			printf("%s\n", errmsg);
			send(socketfd,errmsg,sizeof(strlen(errmsg)),0);
			send(socketfd,&end,sizeof(end),0);
		}
		else
		{
			printf("Query the top 10 users done.\n");
			send(socketfd,&end,sizeof(end),0);
		}
	}

	else if(msg->search_mode == 3) // 管理员查询后10个用户信息
	{
		if(sqlite3_exec(db, "select * from usr order by account desc limit 10;", userinfo_callback,(void *)&socketfd, &errmsg)!= SQLITE_OK)
		{
			printf("%s\n", errmsg);
			send(socketfd,errmsg,sizeof(strlen(errmsg)),0);
			send(socketfd,&end,sizeof(end),0);
		}
		else
		{
			printf("Query The last 10 users done.\n");
			send(socketfd,&end,sizeof(end),0);
		}
	}
	else if(msg->search_mode == 4) //按编号查询
	{
		sprintf(sql,"select * from usr order by account limit %d,%d;",msg->start_num_search-1,((msg->end_num_search)-(msg->start_num_search)+1));
		if(sqlite3_exec(db, sql, userinfo_callback,(void *)&socketfd, &errmsg)!= SQLITE_OK)
		{
			printf("%s\n", errmsg);
			send(socketfd,errmsg,sizeof(strlen(errmsg)),0);
			send(socketfd,&end,sizeof(end),0);
		}
		else
		{
			printf("Query users done.\n");
			send(socketfd,&end,sizeof(end),0);
		}
	}
		
	return 0;
}

int do_login_history(int socketfd,MSG *msg, sqlite3 *db)//查询历史记录
{
	char sql[128] = {0};
	char *errmsg;
	char end = '\0';
	if(msg->history_mode==1) //查询全部历史记录
	{

		if(sqlite3_exec(db, "select * from record;", history_callback,(void *)&socketfd, &errmsg)!= SQLITE_OK)
		{
			printf("%s\n", errmsg);
			send(socketfd,errmsg,sizeof(strlen(errmsg)),0);
			send(socketfd,&end,sizeof(end),0);
		}
		else
		{
			printf("Query all record done.\n");
			send(socketfd,&end,sizeof(end),0);
		}
	}

	else if(msg->history_mode==2) //查询后50个历史记录
	{
		if(sqlite3_exec(db, "select * from record order by id desc limit 10;", history_callback,(void *)&socketfd, &errmsg)!= SQLITE_OK)
		{
			printf("%s\n", errmsg);
			send(socketfd,errmsg,sizeof(strlen(errmsg)),0);
			send(socketfd,&end,sizeof(end),0);
		}
		else
		{
			printf("Query The last 50 historical records done.\n");
			send(socketfd,&end,sizeof(end),0);
		}

	}

	else if(msg->history_mode == 3) //按编号查询
	{

		sprintf(sql,"select * from record order by id limit %d,%d;",msg->start_num_history-1,((msg->end_num_history)-(msg->start_num_history)+1));
		if(sqlite3_exec(db, sql, history_callback,(void *)&socketfd, &errmsg)!= SQLITE_OK)
		{
			printf("%s\n", errmsg);
			send(socketfd,errmsg,sizeof(strlen(errmsg)),0);
			send(socketfd,&end,sizeof(end),0);
		}
		else
		{
			printf("Query history done.\n");
			send(socketfd,&end,sizeof(end),0);
		}
	}

	return 0;

}

int do_exit(MSG *msg, sqlite3 *db)
{
	char sql[500] = {0};
	char *errmsg;

	time_t systime;
	time(&systime);
	sprintf(exittime,"%s", asctime(gmtime(&systime)));
	sprintf(sql, "insert into record values (NULL,'%s','%s','%s');",loginaccount,logintime,exittime);


	
	if(sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK)
	{	
		printf("%s\n", errmsg);
		return -1;
	}
	else
	{
		printf("exit time record successfully.\n");
	}

	sqlite3_exec(db,"update record set logintime =replace(logintime, x'0a','');",NULL, NULL, &errmsg);//去掉换行符
	sqlite3_exec(db,"update record set exittime =replace(exittime, x'0a','');",NULL, NULL, &errmsg);//去掉换行符

	return 0;
}

int do_login(int socketfd,MSG *msg, sqlite3 *db)
{
	char sql[500] = {0};
	char *errmsg;
	int nrow;
	int ncloumn;
	char **resultp;
	time_t systime;
	time(&systime);
	sprintf(logintime,"%s", asctime(gmtime(&systime)));

	sprintf(sql, "select * from usr where account = '%s' and data = '%s';", msg->account, msg->data);
	printf("%s\n", sql);

	if(sqlite3_get_table(db, sql, &resultp, &nrow, &ncloumn, &errmsg)!= SQLITE_OK)
	{
		printf("%s\n", errmsg);
		return -1;
	}
	else
	{
		printf("get_table ok!\n");
	}

	printf("msg->account:%s\n",msg->account);
	printf("msg->data:%s\n",msg->data);
	if(nrow == 1)
	{
		sprintf(sql, "select type from usr where account = '%s';",msg->account);
		sqlite3_get_table(db, sql, &resultp, &nrow, &ncloumn, &errmsg);
		printf("type:%s\n",resultp[1]);
		send(socketfd,resultp[1], sizeof(char), 0);
		strcpy(loginaccount,msg->account);//记录登录时的账号，以便写入登录日志
		return 1;
	}

	else if(nrow == 0) // 密码或者用户名错误
	{
		send(socketfd, &nrow, sizeof(int), 0);
	}

	return 0;
}


int history_callback(void* arg,int f_num,char** f_value,char** f_name)
{
	char str[128]={0};
	int fd;
	fd = *((int *)arg);


	sprintf(str,"NO.:%-2s account: %-8slogintime: %s  exittime: %s",f_value[0],f_value[1],f_value[2],f_value[3]);
	send(fd,str,sizeof(str),0);

	return 0;
}

int userinfo_callback(void* arg,int f_num,char** f_value,char** f_name)
{

	char str[512]={0};
	int fd;
	fd = *((int *)arg);

	sprintf(str,"account:%s\npassword:%s\nusertype:%s\nname:%s\nid:%s\nage:%s\nsalary:%s\naddress:%s\n",f_value[0],f_value[1],f_value[2],f_value[3],f_value[4],f_value[7],f_value[5],f_value[6]);
	send(fd,str,sizeof(str),0);
	return 0;
}




