all: server client

client:	common.o common.h client.o
	gcc -g -o rcat common.c client.c

server: common.o common.h server.o
	gcc -g -o rcatserver common.c server.c

clean:
	rm rcat rcatserver *.o

