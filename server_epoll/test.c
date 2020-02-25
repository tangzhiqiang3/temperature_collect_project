/*********************************************************************************
 *      Copyright:  (C) 2019 Tang Zhiqiang<t_zhiqiang@163.com>
 *                  All rights reserved.
 *
 *       Filename:  test.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(11/21/2019)
 *         Author:  Tang Zhiqiang <t_zhiqiang@163.com>
 *      ChangeLog:  1, Release initial version on "11/21/2019 08:51:48 PM"
 *                 
 ********************************************************************************/

#include <stdio.h>
#include <sqlite3.h>

    static int callback(void *NotUsed, int argc, char **argv, char **azColName)
        {
                    int i;                                
                    for(i=0; i<argc; i++)
                                {
                                                printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");               
                                                        }
                            printf("\n");
                                    
                                }
                                
    
int main(int argc, char* argv[])
{
    sqlite3 *db;
    char    *zErrMsg = 0;
    int     rc;
    char    *sql;
    const char* data = "Callback function called";

    rc = sqlite3_open("test.db", &db);
    if( rc )
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return -1;

    }
    else
    {
        fprintf(stderr, "Opened database successfully\n");
    }
   // sqlite3_close(db);
    sql = "CREATE TABLE COMPANY("  \
        "ID INT PRIMARY KEY     NOT NULL," \
        "NAME           TEXT    NOT NULL," \
        "AGE            INT     NOT NULL," \
        "ADDRESS        CHAR(50)," \
        "SALARY         REAL );";
    
    rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
    if( rc != SQLITE_OK )
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return -1;
    }
    printf("Create table successfully!\n");

    
//    sql = "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY) "  \
//                       "VALUES (1, 'Paul', 32, 'California', 20000.00 ); " \
//                                "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY) "  \
//                                         "VALUES (2, 'Allen', 25, 'Texas', 15000.00 ); "     \
//                                                  "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY)" \
//                                                           "VALUES (3, 'Teddy', 23, 'Norway', 20000.00 );" \
//                                                                    "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY)" \
//                                                                             "VALUES (4, 'Mark', 25, 'Rich-Mond ', 65000.00 );";
//    
       rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
       if( rc != SQLITE_OK )
       {
           fprintf(stderr, "SQL error: %s\n", zErrMsg);
           sqlite3_free(zErrMsg);
       }
       
       fprintf(stdout, "Records created successfully\n");
       //sqlite3_close(db);
          
//    sql = "SELECT * from COMPANY";
//
//    /*  Execute SQL statement */
//    rc = sqlite3_exec(db, sql, callback, (void *)data, &zErrMsg);
//    if( rc != SQLITE_OK )
//    {
//        fprintf(stderr, "SQL error: %s\n", zErrMsg);
//        sqlite3_free(zErrMsg);
//
//    }
//    else
//    {
//        //fprintf(stdout, "Table created successfully\n");
//         fprintf(stdout, "Operate done successfully\n");
//    }
    sqlite3_close(db);

    return 0;
}
    
