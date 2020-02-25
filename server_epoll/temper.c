/*********************************************************************************
 *      Copyright:  (C) 2019 Tang Zhiqiang<t_zhiqiang@163.com>
 *                  All rights reserved.
 *
 *       Filename:  temper.c
 *    Description:  This file is temperature link database
 *                 
 *        Version:  1.0.0(11/21/2019)
 *         Author:  Tang Zhiqiang <t_zhiqiang@163.com>
 *      ChangeLog:  1, Release initial version on "11/21/2019 03:25:39 PM"
 *                 
 ********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>

int main(int argc, char *argv[])
{
    sqlite3  *db=NULL;
    char    *zErrMsg = NULL;
    int     rc;
    int     len;
    char    *sql=NULL;
    char    sql1[128];

    /*  Open database  若没有则创建*/
    len = sqlite3_open("temper.db", &db);
    if(len != SQLITE_OK)
    {
        sqlite3_close(db);
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        exit(1);
    }
    printf("Opened database successfully\n");

    /*  Create SQL statement */
    sql = "create table if not exists temperature(ID char(10), datetime char(50), temperature  char(10))"; 
                    
    /*  Execute SQL statement */
    rc = sqlite3_exec(db, sql, 0, 0, &zErrMsg);
    if( rc != SQLITE_OK )
    {
        sqlite3_close(db);
        fprintf(stderr, "Create table error: %s\n", zErrMsg);
        return -1;
    }
    printf("Table created successfully\n");

    sqlite3_close(db);

    return 0;
}

