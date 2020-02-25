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
#include <pthread.h>
//#include "server_init.c"

//#define MAX_EVENTS      512
//#define ARRAY_SIZE(x)   (sizeof(x)/sizeof(x[0]))    //计算结构体/数组 元素个数
typedef void *(THREAD_BODY) (void *thread_arg);
#define database_name    "temper.db"

static inline void print_usage(char *programe);
//void set_socket_rlimit(void);
void create_database(void);
void *thread_worker(void *ctx);
int thread_start(pthread_t * thread_id, THREAD_BODY * thread_workbody, void *thread_arg);

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
    //int     rv=0;
    int     log_fd;
    //int     i;
    //char    buf[1024];
    pthread_t tid;
    //pid_t   pid;
    struct sockaddr_in  serv_addr;
    int     on=1;
    
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
    //creat database and start insert data of cilent 
    printf("Start create database\n");
    create_database();  
    
    //printf("programe will start running...\n");
    while(!g_sigstop)
    {
        //program will blocked here
        printf("Start accept new cilent...\n");
        connfd=accept(listenfd, (struct sockaddr *)&serv_addr, &cliaddr_len);
        if(connfd < 0)
        {
            printf("accept new client failure: %s\n", strerror(errno));
            continue;
        }
        printf("accept new client [%s:%d] sucessful.\n", inet_ntoa(serv_addr.sin_addr), ntohs(serv_port));
          
        /* !!!! Think about here: why pass clifd but not &clifd ? !!! */
        thread_start(&tid, thread_worker, (void *)connfd);           //thread process turn to thread_worker
    }
    close(listenfd);
    return 0;
}//main()

int thread_start(pthread_t * thread_id, THREAD_BODY * thread_workbody, void *thread_arg)
{
    int rv = -1;
    pthread_attr_t thread_attr;
    if( pthread_attr_init(&thread_attr) )
    {
        printf("pthread_attr_init() failure: %s\n", strerror(errno));
        goto CleanUp;
    }
    if( pthread_attr_setstacksize(&thread_attr, 120*1024) )                     //change thread attribution
    {
        printf("pthread_attr_setstacksize() failure: %s\n", strerror(errno));
        goto CleanUp;
    }
    if( pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED) )    //set detached state
    {
        printf("pthread_attr_setdetachstate() failure: %s\n", strerror(errno));
        goto CleanUp;
    }
    /* Create the thread */
    if( pthread_create(thread_id, &thread_attr, thread_workbody, thread_arg) )  // thread_workbody：function pointer
    {
        printf("Create thread failure: %s\n", strerror(errno));
        goto CleanUp;
    }
    rv = 0;
    CleanUp:
    /* Destroy the attributes of thread */
    pthread_attr_destroy(&thread_attr);
    return rv;
}

void *thread_worker(void *ctx)
{
    char    delim[]="/";
    char    *ptr=NULL;
    int     connfd;
    int     rv;
    char    buf[1024];
    char    sql1[28];
    char    id[20];
    char    data_time[50];
    char    temper[10];
    int     ret;
    char    *zErrMsg = NULL;
    sqlite3  *db;
    int     len;

    if( !ctx )
    {
        printf("Invalid input arguments in %s()\n", __FUNCTION__);
        pthread_exit(NULL);
    }
    connfd = (int)ctx;   //传入的是accept()返回的fd的值

     /*  Open database  */
    len = sqlite3_open(database_name, &db);
    if(len != SQLITE_OK)
    {
        sqlite3_close(db);
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));                               
		pthread_exit(NULL);     //子线程不能调用exit(0)
    }
    printf("Opened database successfully\n");
    /* child thread close the listen socket fd */  
    printf("Child thread start to commuicate with socket client...\n");
    while(1)
	{
		memset(buf, 0, sizeof(buf));
		rv=read(connfd, buf, sizeof(buf)); 
		if(rv <= 0)
		{
            printf("read client [%d] data failure or socket disconnected: %s\n", connfd, strerror(errno));
			close(connfd);              //关闭当前连接客户端的fd
			pthread_exit(NULL);         //退出子线程，不能用exit(0),会导致进程退出
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
				strncpy(temper, ptr, sizeof(temper));
				ptr=strtok(NULL, delim);
			}
            //printf("temper=%s",temper);
			memset(sql1, 0, sizeof(sql1));
			snprintf(sql1, 128, "insert into temperature values('%s', '%s', '%s');", id, data_time, temper);
			//保证了数组sql1的内容为字符串
			sql1[127] = '\0';    
			printf("%s\n", sql1); 
			//调用sqlite3_exec()；将数据存储至temperature表中
			ret = sqlite3_exec(db, sql1, 0 , 0, &zErrMsg);
			if (ret != SQLITE_OK)     //判断返回值，如果不等于SQLITE_OK，即插入记录失败            
			{
				sqlite3_close(db);
				printf("insert data failure: %s!\n", zErrMsg);
                close(connfd);                                
				pthread_exit(NULL);
			}
			printf("insert data successfully!\n");
		}//else if(rv > 0)
	}//while(1)

    close(connfd); 
    sqlite3_close(db);  //关闭数据库
    //return;
}

void create_database(void)
{  
    sqlite3  *db=NULL;
    char    *zErrMsg = NULL;
    int     rc;
    int     len;
    //char  sql ="create table if not exists temperature(ID char(10), datetime char(50), temperature  char(10))";
    char    *sql ="create table temperature(ID char(10), datetime char(50), temperature  char(10))";    //描述创建数据库中表的信息

    /*  Open database  若没有则创建*/
    len = sqlite3_open(database_name, &db);
    if(len != SQLITE_OK)
    {
        sqlite3_close(db);
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));                               
		pthread_exit(NULL);     //子线程不能调用exit(0)
    }
    printf("Opened database successfully\n");
    
    /*  Execute SQL statement */
    rc = sqlite3_exec(db, sql, 0, 0, &zErrMsg);
    if( rc != SQLITE_OK )
    {
        sqlite3_close(db);
        fprintf(stderr, "Create table error: %s\n", zErrMsg);
        pthread_exit(NULL);
    }
    printf("Table created successfully\n");
    //return db;
} 

static inline void print_usage(char *progname)
{
	printf("Usage: %s [OPTION]...\n", progname);
	printf(" %s is a socket server program, which used to verify client and echo back string from it\n",progname);
	printf("\nMandatory arguments to long options are mandatory for short options too:\n");
	printf(" -d[daemon ] set program running on background\n");
	printf(" -p[port ] Socket server port address\n");
	printf(" -h[help ] Display this help information\n");
    //printf(" -t[time ] Set collect data interval time\n")
	printf("\nExample: %s -p 8900 -i 192.168.174.5 -d\n", progname);
	return ;
}

