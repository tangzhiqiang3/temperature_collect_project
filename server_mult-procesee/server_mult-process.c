/*********************************************************************************
 *      Copyright:  (C) 2019 Tang Zhiqiang<t_zhiqiang@163.com>
 *                  All rights reserved.
 *
 *       Filename:  socket_server_RPI.c
 *    Description:  This file is socket server RPI temperature
 *                 
 *        Version:  1.0.0(11/04/2019)
 *         Author:  Tang Zhiqiang <t_zhiqiang@163.com>
 *      ChangeLog:  1, Release initial version on "11/04/2019 08:15:46 PM"
 *                 
 ********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <pthread.h>
#include <getopt.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/epoll.h> 
#include <sys/resource.h>
#include <signal.h>
#include <syslog.h>
#include <fcntl.h>
#include <sqlite3.h>
//#include "server_init.c"

#define MAX_EVENTS      512
#define ARRAY_SIZE(x)   (sizeof(x)/sizeof(x[0]))    //计算结构体数组大小

static inline void print_usage(char *programe);
//void set_socket_rlimit(void);

int     g_sigstop = 0;
void signal_stop(int signum)
{
    if(SIGTERM == signum)
    {
        printf("SIGTERM signal detected\n");
        g_sigstop = 1;
    }
}

int main(int argc, char *argv[])
{
    int     listenfd,connfd;
    int     serv_port = 0;
    char    *serv_ip=NULL;
    int     daemon_run = 0;
    char    *progname = NULL;
    int     opt;
    socklen_t    cliaddr_len=20;
    int     rv=0;
    int     log_fd;
    int     i;
    char    buf[1024];
    char    sql1[28];
    pid_t   pid;
    struct sockaddr_in  serv_addr;	//在main()中，用到该局部变量
    int     on=1;

    sqlite3  *db=NULL;
    char    *zErrMsg = NULL;
    int     rc;
    int     len;
    int     ret;
    char    *sql=NULL;
    char    delim[]="/";
    char    id[20];
    char    data_time[50];
<<<<<<< HEAD
    char    temper[10];
=======
    float   temper[10];
>>>>>>> 77afb4cfb4f84571dd147cdee5d8bf98337c0029
    char    *ptr=NULL;
    //char    *p=NULL;
    
    struct option long_options[]=
    {
        {"daemon", no_argument, NULL, 'd'},
        {"port", required_argument, NULL, 'p'},
        {"help", no_argument, NULL, 'h'},
        {NULL, 0, NULL, 0}
    };

    progname = basename(argv[0]);

    //Parser the command line parameters 
    while((opt = getopt_long(argc, argv, "dp:h", long_options, NULL)) != -1)
    {
        switch (opt) 
        {
            case 'd':
                daemon_run = 1;
                break;
            case 'p':
                serv_port = atoi(optarg);
                break;
            case 'h':
                print_usage(progname);
                break;
            default:
                break;
        }
    }
    if(!serv_port) 
    {
        print_usage(progname);
        return -1;
    }
    
    signal(SIGTERM, signal_stop);   //15号信号（SIGTERM）可被捕捉

    if(daemon_run)
    {
        printf("program %s running at the backgroud now\n", argv[0]);
        
        //建立日志系统
        log_fd=open("receive_temper.log", O_CREAT|O_RDWR, 0666);
        if(log_fd < 0) 
        {
            printf("Open the logfile failure: %s\n", strerror(errno));
            return 0;
        }
        //标准输出、标准出错重定向
        dup2(log_fd, STDOUT_FILENO);
        dup2(log_fd, STDERR_FILENO);

        //程序后台运行
        if(daemon(1, 1) <0 )
        {
            printf("Deamon failure: %s", strerror(errno));
            return 0;
        }
    }
        
   if((listenfd=socket(AF_INET, SOCK_STREAM, 0)) < 0 )
   {
	   printf("create socket failure: %s\n", strerror(errno));
	   return -1;
   }
   //Set socket port reuseable, fix 'Address already in use' bug when socket server restart
   //setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
   memset(&serv_addr, 0, sizeof(serv_addr));
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_port = htons(serv_port);
   if(!serv_ip)
   {
	   /*  监听所有的客户端的IP地址 */
	   serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
   }
   else
   { 
	   if(inet_pton(AF_INET, serv_ip, &serv_addr.sin_addr) <= 0)
	   {
		   printf("inet_pton() set listen IP failure: %s\n", strerror(errno));
		   close(listenfd);
		   //goto Cleanup;
		   return -1;
	   }	   
   }
 
   //Set socket port reuseable, fix 'Address already in use' bug when socket server restart
   if((setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (void*)&on, sizeof(on))) < 0 )
   {
	   printf("setsockopt failure: %s\n", strerror(errno));
	   return -2;
   }

   if(bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
   {
	   printf("bind socket[%d] failure: %s\n", listenfd, strerror(errno));
	   close(listenfd);
	   return -3;	   
   }
   if(listen(listenfd, 64) < 0)
   {
	   //printf("listen() socket[%d] failure: %s\n", listenfd, strerror(errno));
	   printf("ERROR: %s server listen on port %d failure\n", argv[0], serv_port);
	   close(listenfd);
	   return -4;
    }
    printf("server socket[%d] start to listen on port %d\n", listenfd, serv_port);
    
     /*   Open database  若没有则创建*/
    len = sqlite3_open("temper.db", &db);
    if(len != SQLITE_OK)
    {
        sqlite3_close(db);
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        exit(1);
    }
    printf("Opened database successfully\n");
    
     /*   Create SQL statement */
    //sql ="create table if not exists temperature(ID char(10), datetime char(50), temperature  char(10))";
    sql ="create table temperature(ID char(10), datetime char(50), temperature  char(10))";

    /*   Execute SQL statement */
    rc = sqlite3_exec(db, sql, 0, 0, &zErrMsg);
    if( rc != SQLITE_OK )
    {
        sqlite3_close(db);
        fprintf(stderr, "Create table error: %s\n", zErrMsg);
        return -1;
    }
    printf("Table created successfully\n");
   
    //printf("programe will start running...\n");
    while(!g_sigstop)
    {   
        //program will blocked here
        printf("Start accept new cilent...\n");
<<<<<<< HEAD
        connfd=accept(listenfd, (struct sockaddr *)&serv_addr, &cliaddr_len);   //accepte() 第三个参数可以是siziof(struct sockaddr)
=======
        connfd=accept(listenfd, (struct sockaddr *)&serv_addr, &cliaddr_len);
>>>>>>> 77afb4cfb4f84571dd147cdee5d8bf98337c0029
        if(connfd < 0)
        {
            printf("accept new client failure: %s\n", strerror(errno));
            continue;
        }
        printf("accept new client [%s:%d] sucessful.\n", inet_ntoa(serv_addr.sin_addr), ntohs(serv_port));
        
        pid=fork();
        if(pid < 0)
        {
            printf("fork() create child process failure: %s\n", strerror(errno));
            //close(connfd);
            continue;
        }
        else if(pid > 0)
        {
            /* parent process close client fd and goes to accept new cilent again */
            close(connfd);
            continue;
        }
        else if(pid == 0)
        {
            /* child process close the listen socket fd */
            close(listenfd);

            printf("Child process start to commuicate with socket client...\n");
            while(1)
			{
				memset(buf, 0, sizeof(buf));
				rv=read(connfd, buf, sizeof(buf));
				if(rv <= 0)
				{
					printf("read client [%d] data failure or socket disconnected: %s\n", connfd, strerror(errno));
					close(connfd);
					exit(0);
				}
				else if(rv > 0)
				{
					printf("socket[%d] read data: %s\n", connfd, buf);
					//读到数据之后需要处理并保存到数据库
					ptr = strtok(buf, delim);    
					while(ptr != NULL)
					{
						strncpy(id, ptr, sizeof(id));
						ptr=strtok(NULL, delim);
						strncpy(data_time, ptr, sizeof(data_time));
						ptr=strtok(NULL, delim);
						strncpy((char *)temper, ptr, sizeof(temper));
						ptr=strtok(NULL, delim);
					}
<<<<<<< HEAD
					memset(sql1, 0, sizeof(sql1));
					snprintf(sql1, 128, "insert into temperature values('%s', '%s', '%s');", id, data_time, temper);
=======
					memset(buf, 0, sizeof(sql1));
					snprintf(sql1, 128, "insert into temperature values('%s', '%s', '%.2fC');", id, data_time, temper);
>>>>>>> 77afb4cfb4f84571dd147cdee5d8bf98337c0029
					//保证了数组sql1的内容为字符串
					sql1[127] = '\0';    
					printf("%s\n", sql1); 
					//调用sqlite3_exec()；将数据存储至temperature表中
					ret = sqlite3_exec(db, sql1, 0 , 0, &zErrMsg);
					if (ret != SQLITE_OK)     //判断返回值，如果不等于SQLITE_OK，即插入记录失败            
					{
						sqlite3_close(db);
						printf("insert data failure ； %s!\n", zErrMsg);                                  
						return 0;
					}
					printf("insert data successfully!\n");
				}//else if(rv > 0)
			}//while(1)
        }//else if(pid == 0) 
    }//while(!g_sigstop)  
    close(listenfd);
    sqlite3_close(db);  //关闭数据库

    return 0;
}//main()


static inline void print_usage(char *progname)
{
	printf("Usage: %s [OPTION]...\n", progname);
	printf(" %s is a socket server program, which used to verify client and echo back string from it\n",progname);
	printf("\nMandatory arguments to long options are mandatory for short options too:\n");
	printf(" -b[daemon ] set program running on background\n");
	printf(" -p[port ] Socket server port address\n");
	printf(" -h[help ] Display this help information\n");
	printf("\nExample: %s -b -p 8900\n", progname);
	return ;
}

