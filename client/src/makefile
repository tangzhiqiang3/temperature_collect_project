#定义变量指定生成的静态库和动态库的名字
LIBNAME=myclient

#定义一个变量指定库文件和头文件的安装路径 
INSTPATH=`pwd`/../lib/ 

#定义编译器，如果今后交叉编译的话，只需要在这里改成交叉编译器即可
CC=gcc
AR=ar 

# 这里all是整个makefile文件的第一个目标，也就是总的目标，当我们输入make命令时就是要完成这个目标；
all: dynamic_lib  static_lib
	@make clear
	@make install

# dynamic_lib 目标用来编译生成动态库，它是all目标的一个依赖；
dynamic_lib:
	${CC} -shared -fPIC client_init.c get_temper.c get_time.c -o lib${LIBNAME}.so 

# static_lib 目标用来编译生成静态库，它是all目标的一个依赖；
static_lib:
	${CC} -c client_init.c get_temper.c get_time.c
	${AR} -rcs lib${LIBNAME}.a *.o

# install是一个单独的目标，他用来将编译生成的库文件和头文件拷贝到相应的安装路径下。在总目标all下有个动作@make install会执行该目标；
install:
	cp -rf lib${LIBNAME}.*  ${INSTPATH}
	cp -rf *.h ${INSTPATH}

# uninstall是一个单独的目标，他用来在安装路径下删除之前安装的库文件和头文件
uninstall:
	rm -f ${INSTPATH}/*.h	

# clear是一个单独的目标，他用来将编译生成的object临时文件删除。在总目标all下有个动作@make clear会执行该目标； 
clear:
	rm -f *.o

clean: clear 
	rm -f lib${LIBNAME}.*


