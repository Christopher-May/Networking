CC=gcc
CFLAGS= -Wall -pedantic -g -Werror

all: control display server

control: control.o
	$(CC) $(CFLAGS) -o control control.o
display: display.o
	$(CC) $(CFLAGS) -o display display.o
server: server.o
	$(CC) $(CFLAGS) -o server server.o

control.o: control.c
	$(CC) $(CFLAGS) -c control.c
display.o: display.c
	$(CC) $(CFLAGS) -c display.c
server.o: server.c
	$(CC) $(CFLAGS) -c server.c

clean:
	rm -f *.o receiver sender
