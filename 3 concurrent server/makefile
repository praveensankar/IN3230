# the compiler: gcc for C program, define as g++ for C++
CC = gcc

  # compiler flags:
  #  -g    adds debugging information to the executable file
  #  -Wall turns on most, but not all, compiler warnings
CFLAGS  = -g -Wall

all:
	$(CC) server.c -o server
	$(CC) client.c -o client1
	$(CC) client.c -o client2

clean:
	rm -f server client1 client2