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
#include <poll.h>
#include "server_socket_init.h"
#include "create_database.h"

//#define MAX_EVENTS      512
#define ARRAY_SIZE(x)   (sizeof(x)/sizeof(x[0]))    //计算结构体/数组元素个数
#define database_name    "temper.db"
//#define MAX_POLL_FD     1024

//void set_socket_rlimit(void);
int create_database(void);
int server_socket_init(char *listenip, int listen_port);
//int fds_add(struct pollfd *fds, int fd);
//int fds_del(struct pollfd *fds, int fd);

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
    int     found;
    char    buf[1024];
    char    sql1[28];
    //int     fds_array[1024];
    //fd_set  rdset;
    int     maxfd;
    struct pollfd pollfds[1024];
    
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
    //初始化poll结构体
    for(i=0; i<ARRAY_SIZE(pollfds); i++)
    {
        pollfds[i].fd=-1;                //清空数组，全部赋值为-1
        pollfds[i].events = 0;
    }
     /*  将加入 listenfd 到集合中 */
    pollfds[0].fd=listenfd;            //加入监听的文件描述符
    pollfds[0].events=POLLIN;
    maxfd=0;
    //printf("programe will start running...\n");
    while(!g_sigstop)
    {   
        //每次while循环，清空rdset集合，然后把存放在数组当中的有效文件描述符，再加入到集合中

        printf("programe will blocked here...\n");
        //printf("maxfd=%d\n", maxfd);
        rv = poll(pollfds, maxfd+1, -1);           //减少数组的遍历次数
        //printf("rv=%d\n", rv);            //测试poll返回值
        if(rv < 0)
        {
            printf("poll failure: %s\n", strerror(errno));
            break;
        }
        else if(rv == 0)
        {
            printf("poll get timeout\n");
            continue;
        }
        /* listen socket get event means new client start connect now */
        else 
        {
            //printf("rv=%d, maxfd=%d, i=%d\n", rv, maxfd, i);
            //maxfd = i>maxfd ? i : maxfd;
            if(pollfds[0].revents & POLLIN)
            {
                if( (connfd=accept(listenfd, (struct sockaddr *)NULL, NULL)) < 0)   //保存客户端的地址和IP，不感兴趣可以传空
                {
                    printf("accept new client failure: %s\n", strerror(errno));
                    continue;
                }
  
                found = 0;
                for(i=1; i<ARRAY_SIZE(pollfds); i++)
                {   
                if( pollfds[i].fd < 0 )
                {
                    printf("accept new client[%d] and add it into array\n", connfd );
                    pollfds[i].fd = connfd;
                    found = 1;
                    break;
                }
                }
                if( !found )
                {
                    printf("accept new client[%d] but full, so refuse it\n", connfd);
                    close(connfd);
                    continue;
                }	
                maxfd = i>maxfd ? i : maxfd; 
                //printf("rv=%d, maxfd=%d, i=%d\n", rv, maxfd, i);
                if (--rv <= 0)
                    continue; 
                //printf("rv=%d, maxfd=%d\n", rv, maxfd, i);

            }/* listen socket get event means new client start connect now */
            else //data arriver from alrady connected client
            {   
                for(i=1; i<ARRAY_SIZE(pollfds); i++)
                {
                    if( pollfds[i].fd < 0)
                        continue;    
                memset(buf, 0, sizeof(buf));
                if((rv=read(pollfds[i].fd, buf, sizeof(buf))) <= 0 )  //同上
                {
                    printf("socket[%d] read data failure or disconnect and will be remove.\n", pollfds[i].fd);
                    close(pollfds[i].fd);
                    pollfds[i].fd = -1;	
                    continue;
                }
                else 
                {
                    printf("socket[%d] read data: %s\n", pollfds[i].fd, buf);
                    /*  Open database  */
                    len = sqlite3_open(database_name, &db);
                    if(len != SQLITE_OK)
                    {
                        sqlite3_close(db);
                        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));                               
		                close(pollfds[i].fd);            //结束当前链接客户端
                        pollfds[i].fd = -1;
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
                        close(pollfds[i].fd);            //结束当前链接客户端
                        pollfds[i].fd = -1;
                        continue;
                    }
                    printf("insert data successfully!\n");
                }//read data from cilent fd
                }
            }//data arriver from alrady connected client
        }//rv>0
    }//while(!g_sigstop)  
    close(listenfd);
    sqlite3_close(db);  //关闭数据库

    return 0;
}

int fds_add(struct pollfd *fds, int fd)
{
	int i = 1;
	for(; i < ARRAY_SIZE(fds); i++)
	{
		if(fds[i].fd == -1)
		{
			fds[i].fd = fd;
			fds[i].events = POLLIN;
			fds[i].revents = 0;
			break;
		}
	}
}

int fds_del(struct pollfd *fds, int fd)
{
	int i = 1;
	for(; i < ARRAY_SIZE(fds); i++)
	{
		if(fds[i].fd == fd)
		{
			fds[i].fd = -1;
			fds[i].events = 0;
			break;
		}
	}
}