
// Created by praveen on 8/26/21.

#include<stdio.h>

//to use exit function
#include<stdlib.h>

// to use socket facilities
#include <sys/socket.h>

// to use the unix sockets
#include <sys/un.h>

//  defines miscellaneous symbolic constants and types, and declares miscellaneous functions
//  to use read, write, close, unlink
#include <unistd.h>

// to get the socket name
#include "connection.h"

void handle_server(int fd);

void send_message_to_server(int server_fd);

void receive_message_from_server(int fd);

void close_connection(int fd, char actor[10]);

int main(int argc, char *argv[]){
    int   fd;
    struct sockaddr_un  server_address;


    fd = socket(AF_UNIX, SOCK_STREAM, 0);

    server_address.sun_family = AF_UNIX;
    strncpy(server_address.sun_path,SOCKET_NAME, sizeof (server_address.sun_path)-1);

    if(connect(fd, (struct sockaddr *) &server_address, sizeof(server_address)) == -1)
    {
        perror("connect");
        exit(EXIT_FAILURE);
    }
    handle_server(fd);
}

void handle_server(int fd)
{
    int max_buffer_length = MAX_BUFFER_LENGTH;
    char msg[max_buffer_length],buffer[max_buffer_length];

    while(1) {
        send_message_to_server(fd);
        receive_message_from_server(fd);
    }
}

void send_message_to_server(int fd)
{
    char msg[MAX_BUFFER_LENGTH];
    printf("\n enter msg (close - to close the connection) : ");
    bzero(msg, MAX_BUFFER_LENGTH);
    fgets(msg, sizeof(msg), stdin);
    msg[strlen(msg) - 1] = '\0';
    printf(" sent the following message to the server : %s ", msg);
    write(fd, msg, strlen(msg));
    if(strcmp(msg, "close")==0) {
        close_connection(fd, "client");
    }
}



void receive_message_from_server(int fd)
{
    char buffer[MAX_BUFFER_LENGTH];
    bzero(buffer, sizeof(buffer));
    read(fd, buffer, MAX_BUFFER_LENGTH);
    printf("\n Received the following message form server : %s ", buffer);
    if(strcmp(buffer, "close")==0)
    {
        // passive close (close initiated from server)
        close_connection(fd, "server");
    }

}

void close_connection(int fd, char actor[10])
{
    close(fd);
    printf("\n the connection is closed (intiated by the %s ) \n", actor);
    exit(EXIT_SUCCESS);
}