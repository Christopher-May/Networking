CC=gcc
CFLAGS= -Wall -pedantic -g -Werror

all: receiver sender

receiver: receiver.o
	$(CC) $(CFLAGS) -o receiver receiver.o

sender: sender.o
	$(CC) $(CFLAGS) -o sender sender.o


receiver.o:receiver.c
	$(CC) $(CFLAGS) -c receiver.c

sender.o:sender.c
	$(CC) $(CFLAGS) -c sender.c


clean:
	rm -f *.o receiver sender