1. 在运行client_main.c文件时，需要加入当前文件lib的路径，根据自己存放的路径进行更改：
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:当前文件路径pwd得出/lib
2. 程序make之后生成可执行文件，再通过make run 查看如何运行程序