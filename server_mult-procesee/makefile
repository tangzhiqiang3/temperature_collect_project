APPNAME=server_mult-process

CC=gcc

all:
	@echo "Start compile server muilt-process program..."
	${CC}  server_mult-process.c -o ${APPNAME} -lsqlite3

run:
	@echo "Start run test program..." 
	#makefile??????????
	@echo "Mandatory arguments to long options are mandatory for short option too:"
	@echo "-p[port] Socket client port address"
	@echo "-i[ip] Socket client ip address"
	@echo "-h[help] Display this help information"
	@echo "Example: ./${APPNAME} -t 30 -p 8088 -i 192.168.174.5"
	

clean:
	rm -rf ${APPNAME} 
