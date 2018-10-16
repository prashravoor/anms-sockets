#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>

#define DEFAULT_SERVER_PORT 8080
#define SOCKET_DOMAIN AF_INET
#define SOCKET_TYPE SOCK_STREAM
#define MAX 1000

extern int errno;

void error(char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char **argv)
{
    int client_socket, port = DEFAULT_SERVER_PORT;
    struct sockaddr_in server_address;
    int numBytesWritten, numBytesRead;
    char buffer[MAX];

    client_socket = socket(SOCKET_DOMAIN, SOCKET_TYPE, 0);
    if (client_socket < 0)
    {
        error("Failed to create the socket for the client");
    }
    printf("Succesfully created socket: %d\n", client_socket);
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = SOCKET_DOMAIN;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port);

    printf("Attempting to connect to server on port %d\n", port);
    if (connect(client_socket, (const struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        close(client_socket);
        error("Failed to connect to the server");
    }
    printf("Successfully connected to server, Sending the File name\n");
    memset(buffer, 0, sizeof(buffer));
    strncpy(buffer, argv[1], strnlen(argv[1], MAX));
    if (send(client_socket, buffer, strnlen(argv[1], MAX), 0) < 0)
    {
        close(client_socket);
        error("Failed to write the data to the server");
    }

    memset(buffer, 0, sizeof(buffer));
    printf("Attempting to read contents of file\n");
    if (recv(client_socket, buffer, MAX, 0) < 0)
    {
        close(client_socket);
        error("Failed to read file contents");
    }

    printf("Received: %s\n", buffer);

    close(client_socket);
    return 0;
}