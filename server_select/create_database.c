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
#include "create_database.h"

#define database_name    "temper.db"

int create_database(void)
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
		return  -1;  //子线程不能调用exit(0)
    }
    printf("Opened database successfully\n");
    
    /*  Execute SQL statement */
    rc = sqlite3_exec(db, sql, 0, 0, &zErrMsg);
//    if( rc != SQLITE_OK )
//    {
//        sqlite3_close(db);
//        fprintf(stderr, "Create table error: %s\n", zErrMsg);
//        return 0;           //打开一个已创建的数据库，并已插入表，插入重复的表会失败，防止主程序退出
//   }
    printf("Table created successfully\n");

    sqlite3_close(db);
    return 1;
}
