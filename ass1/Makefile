CC=gcc
CFLAGS= -Wall -pedantic -g

all:tcpServer


tcpSever:tcpServer.o
	$(CC) $(CFLAGS) -o tcpSever tcpServer.o

tcpServer.o:tcpServer.c
	$(CC) $(CFLAGS) -c tcpServer.c

clean:
	rm -f *.o tcpServer.o tcpServer
