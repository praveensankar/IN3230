
// Created by praveen on 8/26/21.


#include<stdio.h>

// to use socket facilities
#include <sys/socket.h>

// to use the unix sockets
#include <sys/un.h>

//  defines miscellaneous symbolic constants and types, and declares miscellaneous functions
//  to use read, write, close, unlink
#include <unistd.h>


// to get the socket name
#include "connection.h"

// to use exit declarations
#include <stdlib.h>

int handle_client(int client_fd);
void shutdown_server(int server_fd, int number_of_clients);

int main(int argc, char *argv[])
{
    int server_fd, client_fd, connections = MAX_CONNECTIONS;
    struct sockaddr_un server_address;



    // AF_UNIX - unix socket (also known as AF_LOCAL)
    // SOCK_STREAM - stream socket
    // IPPROTO_TCP - TCP transport protocol
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    // socket() will return -1 if there is any error
    if(server_fd==-1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    server_address.sun_family = AF_UNIX;
    strncpy(server_address.sun_path,SOCKET_NAME, sizeof(server_address.sun_path)-1);

    // binds the socket file descriptor to the socket address
    // sometimes it's better to cast the pointer type of the server address to (struct sockaddr *) explicitly
    bind(server_fd, (const struct sockaddr *)&server_address, sizeof(server_address));

    printf(" socket is listening for new connections \n");

    listen(server_fd, MAX_CONNECTIONS);

    while(connections--) {

        client_fd = accept(server_fd, NULL, NULL);
        int status = handle_client(client_fd);
        if(status == -1 )
            shutdown_server(server_fd, MAX_CONNECTIONS-connections);
    }
    shutdown_server(server_fd, MAX_CONNECTIONS-connections);
}

//client_fd - file descriptor for the socket
int handle_client(int client_fd)
{
    int max_buffer_length = MAX_BUFFER_LENGTH;
    char buffer[max_buffer_length], msg[max_buffer_length];

    while (1) {
        bzero(buffer, sizeof(buffer));
        read(client_fd, buffer, max_buffer_length);
        printf("\n message from client : %s \n", buffer);

        if (strcmp(buffer, "close") == 0) {
            // passive close
            close(client_fd);
            return 1;
        }

        printf("enter message to client : ");
        bzero(msg, sizeof(msg));
        fgets(msg, sizeof(msg), stdin);
        msg[strlen(msg) - 1] = '\0';
        write(client_fd, msg, strlen(msg));

        if (strcmp(msg, "close") == 0) {
            // performs active close and also exit the server socket which is listening for new connections
            // return -1 to the main function to notify that server is going to shut down
            close(client_fd);
            return -1;
        }
    }

}
// server_fd - file descriptor of the server
// number_of_clients  - number of clients handled by the server
void shutdown_server(int server_fd, int number_of_clients)
{
    close(server_fd);
    unlink(SOCKET_NAME);
    printf("\n server is shutting down. No of clients served : %d \n", number_of_clients);
    exit(EXIT_SUCCESS);
}
