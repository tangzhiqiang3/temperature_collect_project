/*********************************************************************************
 *      Copyright:  (C) 2019 Tang Zhiqiang<t_zhiqiang@163.com>
 *                  All rights reserved.
 *
 *       Filename:  socket_client_RPI.c
 *    Description:  This file socket client monitor RPI temperature.
 *                 
 *        Version:  1.0.0(10/22/2019)
 *         Author:  Tang Zhiqiang <t_zhiqiang@163.com>
 *      ChangeLog:  1, Release initial version on "10/22/2019 04:32:59 PM"
 *                 
 ********************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <getopt.h>
#include <syslog.h>
#include <signal.h>
#include <fcntl.h>
#include <sqlite3.h>
#include "header.h"

#define     id  "t_zhiqang"  

int     g_sigstop = 0;

//void print_usage(char *progname);

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
    int                 sock_fd = -1;
    int                 rv = -1;
    struct sockaddr_in  serv_addr;
    char                *serv_ip = NULL;
    int                 serv_port = 0;
    char                buf[1024];
    int                 ch;
    char                da_time[50];                
    int                 interv_time = 0;   
    float               temper;
    int                 daemon_run = 0;
    int                 log_fd = -1;

    static struct option long_options[] = {
            {"d", no_argument, NULL, 'd'},
            {"ipaddr", required_argument, NULL, 'i'},
            {"port", required_argument, NULL, 'p'},
            {"help", no_argument, NULL, 'h'},
            {"interv_time", required_argument, NULL, 't'},
            {0, 0, 0, 0}
        };

    while((ch=getopt_long(argc, argv, "di:p:ht:", long_options, NULL)) != -1)
        {
            switch (ch) 
            {
                case 'd':
                    daemon_run = 1;
                    
                    break;
                case 'i': 
                    serv_ip=optarg;
                    break;
                case 'p': 
                    serv_port=atoi(optarg); //optarg返回的是字符串，atoi()将char转换为int型
                    break;
                case 'h': 
                    print_usage(argv[0]);
                    break;
                case 't':
                    interv_time=atoi(optarg);
                    break;
                default:
                    print_usage(argv[0]);
                    exit(0);
                    break;
            }
            
        }
        if(!serv_ip || !serv_port)
        {
            print_usage(argv[0]);
            return -1;
        }

        //openlog("socket_client_RPI", LOG_CONS | LOG_PID, 0);    //可选
        //printf("Program %s running failure: %s\n",argv[0], strerror(errno)); //将错误或调试信息写入系统自带的syslog日志
        
        signal(SIGTERM, signal_stop);   //安装signal信号
        signal(SIGPIPE, SIG_IGN);   //屏蔽SIGPIPE信号，因为当服务器主动断开socket连接后，客户端会接收到SIGPIPE信号，自动退出。
                                    //为达到服务器主动断开socket连接后，客户端重连服务器端的目的，这里必须屏蔽掉SIGPIPE信号

        //守护进程函数
        if (daemon_run)         
        {
            printf("Program %s is running at the background now\n", argv[0]);
            //创建日志系统，程序后台运行后，将所有打印信息打印在日志文件中
            log_fd = open("receive_temper.log",  O_CREAT|O_RDWR, 0666);
            if (log_fd < 0)
            {
                printf("Open the logfile failure : %s\n", strerror(errno));

                return 0;
            }
            //标准输出及标准出错重定向，重定向至日志文件
            dup2(log_fd, STDOUT_FILENO);
            dup2(log_fd, STDERR_FILENO);
            //设置deamon()函数两个参数为1
            //即保持当前目录不变，并且使标准输出及标准出错重定向仍打印输出信息，只不过此时打印信息将会全部打印至日志文件中！
            if ((daemon(1, 1)) < 0)
            {
                printf("Deamon failure : %s\n", strerror(errno));
                return 0;
            }
	} 
<<<<<<< HEAD
=======
	
//	len = sqlite3_open("temper.db", &db);
//	if(len != SQLITE_OK)
//	{
//        	sqlite3_close(db);
//	        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
//	        exit(1);
//	}
//    printf("Opened database successfully\n");
//	
//	/*   Create SQL statement */
//	//sql ="create table if not exists temperature(ID char(10), datetime char(50), temperature  char(10))";
//    sql ="create table temperature(ID char(10), datetime char(50), temperature  char(10))";
//         
//	/*   Execute SQL statement */
//	rc = sqlite3_exec(db, sql, 0, 0, &zErrMsg);
//    if( rc != SQLITE_OK )
//   	{
//        sqlite3_close(db); 
//	 	fprintf(stderr, "Create table error: %s\n", zErrMsg);
//        return -1;
//    }
//    printf("Table created successfully\n");

>>>>>>> 77afb4cfb4f84571dd147cdee5d8bf98337c0029
    while (!g_sigstop) 
    {   
            /*采样SN、时间、温度...*/  
            get_time(da_time);
            if((get_temper(&temper)) < 0)
	        {
	    	    printf("Get temperature failure: %s\n", strerror(errno));
		        continue;
	        }
            
            memset(buf, 0, sizeof(buf));
            snprintf(buf, sizeof(buf), "%s/%s/%.2f%c", id, da_time, temper, 'C');
            
<<<<<<< HEAD
=======
//            memset(buf, 0, sizeof(sql1));
//            snprintf(sql1, 128, "insert into temperature values('%s', '%s', '%.2fC');", id, da_time, temper);
//            //保证了数组sql1的内容为字符串
//            sql1[127] = '\0';    
//	        printf("%s\n", sql1); 
//                            
//            //调用sqlite3_exec()；将数据存储至temperature表中
//            ret = sqlite3_exec(db, sql1, 0 , 0, &zErrMsg);
//            if (ret != SQLITE_OK)     //判断返回值，如果不等于SQLITE_OK，即插入记录失败            
//            {
//                sqlite3_close(db);
//            
//                printf("insert data failure ； %s!\n", zErrMsg);                                  
//                return 0;
//            }
//            printf("insert data successfully!\n");
            
>>>>>>> 77afb4cfb4f84571dd147cdee5d8bf98337c0029
            //未连接到服务器
            if(sock_fd < 0)
            {   
                //调用client_ini()函数，连接服务器
                if((sock_fd=client_init(serv_port, serv_ip)) < 0)    
                {
                    printf("connect server point  fialure: %s\n", strerror(errno));
                    continue;   //连接服务端失败，重新连接
                }
            }
            /* connect server OK！*/
            if(sock_fd >= 0)
            {
                if(write(sock_fd, buf, sizeof(buf)) < 0)
                {
                    printf("write data to server fialure: %s\n", strerror(errno));
                    close(sock_fd);
                    sock_fd = -1;
                    return -3;
                }
            }
            printf("Send messege to server sucessfully!\n");

            sleep(interv_time); //命令行参数设定上传数据间隔时间   
        }

        //closelog();
        close(sock_fd);
	    //sqlite3_close(db);

        return 0;
}

<<<<<<< HEAD
=======

>>>>>>> 77afb4cfb4f84571dd147cdee5d8bf98337c0029
void print_usage(char *progname)
{
    printf("%s usage.\n", progname);     
    printf("%s is a socket client progname, which used to verify server and echo back string from it\n", progname);
    printf("\nMandatory arguments to long options are mandatory for short option too:\n");
    printf("-b[darmon] set program running on backgroud\n");
    printf("-t[interv_time] RPI temperature interval time\n");
    printf("-p[port] Socket client port address\n");
    printf("-i[ip] Socket client ip address\n");
    printf("-h[help] Display this help information\n");
    printf("\nExample: %s -t 30 -p 8088 -i 192.168.174.5\n", progname);
    return;
}


