
// Created by praveen on 8/26/21.


#include<stdio.h>

// to use socket facilities
#include <sys/socket.h>

// to use the unix sockets
#include <sys/un.h>

//  defines miscellaneous symbolic constants and types, and declares miscellaneous functions
//  to use read, write, close, unlink, fork
#include <unistd.h>


// to get the socket name
#include "connection.h"

// to use exit declarations
#include <stdlib.h>

#include<time.h>

void handle_client(int client_fd);
void shutdown_server(int server_fd, int clients);
int send_message_to_client(int fd,  char *username);
int receive_message_from_client(int fd,  char *username);
void assign_user_name(int fd, char *username);


int main(int argc, char *argv[])
{
    int server_fd, client_fd, connections = 0;
    int pid;
    char cwd[MAX_DIR_LENGTH], socket_name[MAX_DIR_LENGTH];
    struct sockaddr_un server_address;


    if (getcwd(cwd, sizeof(cwd)) != NULL) {
       // printf("Current working dir: %s\n", cwd);
    } else {
        perror("getcwd");
        exit(EXIT_FAILURE);
    }
    strcat(cwd, "/");
    strcat(cwd, SOCKET_NAME);
    strcpy(socket_name, cwd);


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
    strncpy(server_address.sun_path,socket_name, sizeof(server_address.sun_path)-1);

    // binds the socket file descriptor to the socket address
    // sometimes it's better to cast the pointer type of the server address to (struct sockaddr *) explicitly
    bind(server_fd, (const struct sockaddr *)&server_address, sizeof(server_address));

    printf(" socket is listening for new connections ");

    listen(server_fd, MAX_CONNECTIONS);
    while(1) {
        client_fd = accept(server_fd, NULL, NULL);
        if(client_fd==-1)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        pid = fork();
        if(pid==-1)
        {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        if(pid==0)
        {
            // it's child process
            // it handles the corresponding client
            // close the copy of the server_fd (listening socket)
            close(server_fd);
            handle_client(client_fd);
            exit(EXIT_SUCCESS);
        }
        if(pid>0)
        {
            connections++;
            if(connections == MAX_CONNECTIONS) {
                // wait for the child processes to finish the requests then shutdown the server
                //shutdown_server(server_fd, connections);

            }
        }
    }
}

//client_fd - file descriptor for the socket
void handle_client(int client_fd)
{

    char *username = (char *)malloc(sizeof(char)* 10);
    strcat(username, "User ");
    assign_user_name(client_fd, username);
    printf("\n %s is connected", username);
    while (1) {
        int status = receive_message_from_client(client_fd, username);
        if(status == -1)
        {
            close(client_fd);
            free(username);
            return;
        }
        int sent_status = send_message_to_client(client_fd, username);
        if( sent_status == -1)
        {
            close(client_fd);
            free(username);
            return;
        }


    }
}
// returns 0 if sent successful
// -1 if server wants to close the connection
// -2 if server wants to shutdown
int send_message_to_client(int fd, char *username)
{
    char msg[MAX_BUFFER_LENGTH];
    printf("\n enter msg (close - to close the connection) to the client %s: ", username);
    bzero(msg, MAX_BUFFER_LENGTH);
    fgets(msg, sizeof(msg), stdin);
    msg[strlen(msg) - 1] = '\0';

    printf(" \n sent the following message to the client %s : %s ",username, msg);
    write(fd, msg, strlen(msg));
    if(strcmp(msg, "close")==0) {
        return -1;
    }
    else
        return 0;
}



int receive_message_from_client(int fd, char *username)
{
    char buffer[MAX_BUFFER_LENGTH];
    bzero(buffer, MAX_BUFFER_LENGTH);
    read(fd, buffer, sizeof(buffer));
    printf("\n Received the following message form client %s : %s ", username, buffer);
    if(strcmp(buffer, "close")==0)
    {
        // passive close (close initiated from server)
        return -1;
    }
    else{
        return 0;
    }
}

// server_fd - file descriptor of the server
// number_of_clients  - number of clients handled by the server
void shutdown_server(int server_fd, int clients_serverd)
{
    close(server_fd);
    unlink(SOCKET_NAME);
    printf("\n server is shutting down. No of clients served : %d\n ", clients_serverd);
    exit(EXIT_SUCCESS);
}

void assign_user_name(int fd, char *username)
{
    char num[3];
    // Use current time as seed for random generator
    srand(time(0));
    sprintf(num, "%d", rand()%10);
    strcat(username, num);
}
