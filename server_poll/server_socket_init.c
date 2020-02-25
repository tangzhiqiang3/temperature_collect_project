#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <sqlite3.h>
#include "server_socket_init.h"

int server_socket_init(char *listenip, int listen_port)
{
        int                 listenfd;
        int                 on = 1;
        int                 rv = 0;
        struct sockaddr_in  serv_addr;


        if((listenfd=socket(AF_INET, SOCK_STREAM, 0)) < 0 )
        {
            printf("create socket failure: %s\n", strerror(errno));
            return -1;
        }
        //Set socket port reuseable, fix 'Address already in use' bug when socket server restart
        //setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(listen_port);
        if( !listenip)
        {
            /*  监听所有的客户端的IP地址 */
            serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        }
        else
        { 
            if(inet_pton(AF_INET,listenip, &serv_addr.sin_addr) <= 0)
            {
                printf("inet_pton() set listen IP failure: %s\n", strerror(errno));
                rv = -2;
                goto Cleanup;
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
            rv = -3;
            goto Cleanup;
        }
        if(listen(listenfd, 64) < 0)
        {
            printf("listen() socket[%d] failure: %s\n", listenfd, strerror(errno));
            rv = -4;
            goto Cleanup;
        }
        Cleanup:
        if(rv < 0) 
        {
            close(listenfd);   
        }
        else 
            rv = listenfd;
        
        return rv;
}
