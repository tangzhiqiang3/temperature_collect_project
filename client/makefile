APPNAME=client_main 

LIBPATH=`pwd`/./lib/

HEADFILE=-I ${LIBPATH}
LIBFILE=-L ${LIBPATH}
STATLIB=-static

CC=gcc

all:
	@echo "Start compile library..."
	make -C src 
	@echo "Start compile client_main program..."
	${CC}  client_main.c -o ${APPNAME} ${HEADFILE}  ${LIBFILE} -lmyclient 


run:
	@echo "Start run test program..." 
	#export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:LIBPATH 
	@echo "example: export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/ten/tangzhiqiang/temperature_project/client/lib" 
	@echo "Mandatory arguments to long options are mandatory for short option too:"
	@echo "-t[interv_time] RPI temperature interval time"
	@echo "-p[port] Socket client port address"
	@echo "-i[ip] Socket client ip address"
	@echo "-h[help] Display this help information"
	@echo "Example: ./client_main -t 30 -p 8088 -i 192.168.174.5"
	

clean:
	make clean -C src 
	rm -rf ${APPNAME} 

distclean: clean 
	rm -rf lib/*.a
	rm -rf lib/*.so 
	rm -rf lib/*.h
