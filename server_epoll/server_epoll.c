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
#include <sys/resource.h>
#include <signal.h>
#include <syslog.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sqlite3.h>
#include <sys/epoll.h> 
#include "server_socket_init.h"
#include "create_database.h"

#define MAX_EVENTS      512
#define ARRAY_SIZE(x)   (sizeof(x)/sizeof(x[0]))    //计算结构体/数组元素个数
#define database_name    "temper.db"

//void set_socket_rlimit(void);
int create_database(void);
int server_socket_init(char *listenip, int listen_port);

int     g_sigstop = 0;
void signal_stop(int signum)
{
    if(SIGTERM == signum)
    {
        printf("SIGTERM signal detected\n");
        g_sigstop = 1;
    }
}

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

int main(int argc, char *argv[])
{
    int     listenfd,connfd;
    int     serv_port = 0;
    int     daemon_run = 0;
    char    *progname = NULL;
    int     opt;
    int     rv;
    int     log_fd;
    int     i;
    //int     found;
    char    buf[1024];
    char    sql1[28];
    
    int     epollfd;
    struct  epoll_event event;
    struct  epoll_event event_array[MAX_EVENTS];
    int     events;
    
    char    *zErrMsg = NULL;
    int     ret;
    char    delim[]="/";
    char    id[20];
    char    data_time[50];
    char    temper[10];
    char    *ptr=NULL;
    sqlite3  *db;
    int     len;
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
    
    if((listenfd=server_socket_init(NULL, serv_port)) < 0)
    {
        printf("ERROR: %s server listen on port %d failure\n", argv[0], serv_port);
        return -2;
    }
    printf("server socket[%d] start to listen on port %d\n", listenfd, serv_port);
    
    printf("Start create database\n");
    if(rv=create_database() < 0)
    {
        printf("Create database failure: %s\n", strerror(errno));
        return -1;
    }  
    printf("Create database OK\n");
    /*  将加入 listenfd 到兴趣数组中 */
    if((epollfd=epoll_create(MAX_EVENTS)) < 0)       //这里缺少括号，优先级出问题了 已改
    {
        printf("%s create epoll farilure: %s\n", argv[0], strerror(errno));
        return -3;
    }
    printf("programe create epoll[%d] OK.\n", epollfd);
    /* 必须将文件描述符赋值给event.data.fd */
    event.events = EPOLLIN; 
    event.data.fd = listenfd;
    if(epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &event) < 0 )
    {
        printf("epoll add listen socket[%d] failure: %s\n", listenfd, strerror(errno));
        return -4;
    }
    
    //printf("programe will start running...\n");
    while(!g_sigstop)
    {   
        //program will blocked here
        printf("programe will blocked here...\n");
        events=epoll_wait(epollfd, event_array, MAX_EVENTS, -1);    //成功则返回数据元素个数
        if(events < 0)
        {
            printf("epoll failure: %s\n", strerror(errno));break;
        }
        else if(events==0) 
        {
            printf("epoll timeout\n");
            continue;
        }
        //events>0 is the active events count
        for(i = 0; i < events; i++)
        {   
            //for循环的events与结构体成员中的events不同
            //按位与求的是结果，逻辑与求的是真或假
            if((event_array[i].events&EPOLLERR) || (event_array[i].events&EPOLLHUP)) 
            {
                printf("epoll_wait get error on fd[%d]: %s\n",event_array[i].data.fd, strerror(errno));
                epoll_ctl(epollfd, EPOLL_CTL_DEL, event_array[i].data.fd, NULL);
                close(event_array[i].data.fd);
            }
            //listen socket get event means new client start connect now
            if(event_array[i].data.fd == listenfd) //已将listenfd加入到epoll感兴趣事件集合
            {
                if((connfd=accept(listenfd, (struct sockaddr *)NULL, NULL)) < 0) //不保存客户端信息（结构体指针为NULL）
                {
                    printf("accept new client failure: %s\n", strerror(errno));      
                    continue;
                }
                event.data.fd = connfd; 
                event.events = EPOLLIN;
                if(epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &event) < 0 )
                {
                   printf("epoll add client socket failure: %s\n", strerror(errno));
                   close(event_array[i].data.fd);
                    continue;
                }
                printf("accept new client[%d] OK\n", connfd);           
            }
            else //data arriver from alrady connected client
            {   
                memset(buf, 0, sizeof(buf));
                if((rv=read(event_array[i].data.fd, buf, sizeof(buf))) <= 0 )  //同上
                {
                     printf("socket[%d] read data failure or disconnect and will be remove.\n", event_array[i].data.fd);
                     epoll_ctl(epollfd, EPOLL_CTL_DEL, event_array[i].data.fd, NULL);
                     close(event_array[i].data.fd);
                     continue;
                }
                else 
                {
                    printf("socket[%d] read data: %s\n", event_array[i].data.fd, buf);
                    /*  Open database  */
                    len = sqlite3_open(database_name, &db);
                    if(len != SQLITE_OK)
                    {
                        sqlite3_close(db);
                        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));   
                        epoll_ctl(epollfd, EPOLL_CTL_DEL, event_array[i].data.fd, NULL);                            
		                close(event_array[i].data.fd);            //结束当前链接客户端
                        continue;    
                    }
                    printf("Opened database successfully\n");
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
                        epoll_ctl(epollfd, EPOLL_CTL_DEL, event_array[i].data.fd, NULL);
                        close(event_array[i].data.fd);
                        continue;
                    }
                    printf("insert data successfully!\n");
                }//read data from cilent fd
            } //for(i=0; i<ARRAY_SIZE(fds_array); i++)
        }//data arriver from alrady connected client
    }//while(!g_sigstop)  
    close(listenfd);
    sqlite3_close(db);  //关闭数据库

    return 0;
}
