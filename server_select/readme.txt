1. 将创建数据库封装成函数，使用时，只需要直接打开数据库的文件即可
2. 数据库文件名称通过宏定义更改
3. create_database*  server_socket_init* server_select.c 文件放在一个路径中，并且将三个.c文件同时gcc编译，makefile已加入