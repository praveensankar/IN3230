
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
        printf("\n enter msg (close - to close the connection) : ");
        bzero(msg, sizeof(msg));
        fgets(msg, sizeof(msg), stdin);
        msg[strlen(msg) - 1] = '\0';

        // if the client enters "close" in the input, close the socket
        if(strcmp(msg, "close")==0)
        {
            write(fd, msg, strlen(msg));
            printf(" sent the 'close' message to the server");
            // performs active close | step 1 in TCP close protocol
            close(fd);
            printf("\n the connection is closed (intiated by the client) \n");
            exit(EXIT_SUCCESS);
        }
        printf(" sent the following message to the server : %s ", msg);
        write(fd, msg, strlen(msg));

        bzero(buffer, sizeof(buffer));
        read(fd, buffer, max_buffer_length);
        printf("\n Received the following message form server : %s ", buffer);
        if(strcmp(buffer, "close")==0)
        {
            // passive close (close initiated from server)
            close(fd);
            printf("\n the connection is closed (intiated by the server) \n ");
            break;
        }
    }
    exit(EXIT_SUCCESS);
}