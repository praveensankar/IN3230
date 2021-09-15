CC = gcc
CFLAGS  = -g -Wall
all:
	$(CC) $(CFLAGS) receiver.c common.c -o receiver
	$(CC) $(CFLAGS) sender.c common.c -o sender

clean:
	rm -f sender receiver

