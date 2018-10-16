#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_SERVER_PORT 8080
#define SOCKET_DOMAIN AF_INET
#define SOCKET_TYPE SOCK_STREAM
#define MAX 1000

extern int errno;

void serveFile(int server_socket, int client_socket);

void error(char *msg)
{
    perror(msg);
    exit(1);
}

int main()
{
    int server_socket, client_socket, port = DEFAULT_SERVER_PORT;
    struct sockaddr_in server_address, client_address;
    socklen_t client_len = 0;

    server_socket = socket(SOCKET_DOMAIN, SOCKET_TYPE, 0);
    if (server_socket < 0)
    {
        error("Failed to create the socket for the server");
    }
    printf("Succesfully created socket: %d\n", server_socket);

    memset(&server_address, 0, sizeof(server_address));
    memset(&client_address, 0, sizeof(client_address));
    server_address.sin_family = SOCKET_DOMAIN;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port);

    if (bind(server_socket, (const struct sockaddr*)&server_address, sizeof(server_address)))
    {
        error("Failed to bind the socket to the server");
    }
    printf("The server is bound to port %d\n", port);
    while(1) {
        if(listen(server_socket, 0)) {
            error("Listening for connections interrupted");
        }
        printf("Server is now listening for connections\n");
        client_socket = accept(server_socket, (struct sockaddr*)&client_address, &client_len);
        printf("Connection received: %d\n", client_socket);
        serveFile(server_socket, client_socket);
        close(client_socket);
    }

    close(server_socket);
    return 0;
}

void serveFile(int server_socket, int client_socket) {
    char buffer[MAX];
    int numBytesRead = 0;
    printf("Attempting to read file name from socket %d\n", client_socket);
    memset(buffer, 0, sizeof(buffer));

    numBytesRead = recv(client_socket, buffer, MAX, 0);
    if(numBytesRead < 0) {
        perror("Failed to read the file name from the client");
        close(client_socket);
        //close(server_socket);
        return;
    }

    printf("Attempting to read file %s\n", buffer);
    printf("Writing file to client\n");
    if(send(client_socket, buffer, numBytesRead, 0) < 0 ) {
        perror("Failed to send the data to the client");
    } else {
        printf("Successfully served client %d\n", client_socket);
    }

    close(client_socket);
    //close(server_socket);
    return;
}