#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#define DEFAULT_SERVER_PORT 8080
#define DEFAULT_QUEUE_SIZE 5
#define SOCKET_DOMAIN AF_INET
#define SOCKET_TYPE SOCK_STREAM
#define MAX 1000

extern int errno;

void serveFile(int server_socket, int client_socket);
int openFileForRead(char *name);

void error(char *msg, int socket)
{
    perror(msg);
    if (socket > 0)
    {
        close(socket);
    }
    exit(1);
}

int main(int argc, char **argv)
{
    int server_socket, client_socket, port = DEFAULT_SERVER_PORT, maxQueueSize = DEFAULT_QUEUE_SIZE;
    struct sockaddr_in server_address, client_address;
    char *ipAddress = NULL;
    socklen_t client_len = 0;

    // Usage: rcatserver <Optional IP> <Optional Port> <Optional Max Queue Size>
    if (argc != 1 && argc != 4)
    {
        printf("Invalid command syntax\n\nUsage: %s <Optional IP> <Optional Port> <Optional Max Queue Size>\n", argv[0]);
        return 1;
    }

    if (4 == argc)
    {
        ipAddress = argv[1];
        port = atoi(argv[2]);
        if (port < 1024)
        {
            printf("Port number is not valid, specify a port value >= 1024\n");
            return 1;
        }

        maxQueueSize = atoi(argv[3]);
        if (maxQueueSize < 0 || maxQueueSize > 5)
        {
            printf("Invalid value for max queue size, enter a value between 0 and 5\n");
            return 1;
        }
    }

    server_socket = socket(SOCKET_DOMAIN, SOCKET_TYPE, 0);
    if (server_socket < 0)
    {
        error("Failed to create the socket for the server", server_socket);
    }
    printf("Succesfully created socket: %d\n", server_socket);

    memset(&server_address, 0, sizeof(server_address));
    memset(&client_address, 0, sizeof(client_address));
    server_address.sin_family = SOCKET_DOMAIN;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port);

    if (bind(server_socket, (const struct sockaddr *)&server_address, sizeof(server_address)))
    {
        error("Failed to bind the socket to the server", server_socket);
    }

    printf("The server is bound to port %d\n", port);
    if (listen(server_socket, maxQueueSize))
    {
        error("Listening for connections interrupted", server_socket);
    }

    while (1)
    {
        printf("Server is now listening for connections\n");
        client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_len);
        printf("Connection received: %d\n", client_socket);
        if (client_socket < 0)
        {
            perror("Failed to accept the connection");
            continue;
        }
        serveFile(server_socket, client_socket);
        close(client_socket);
    }

    close(server_socket);
    return 0;
}

void serveFile(int server_socket, int client_socket)
{
    char buffer[MAX], *error = NULL;
    int numBytesRead = 0, fd = 0;
    printf("Attempting to read file name from socket %d\n", client_socket);
    memset(buffer, 0, sizeof(buffer));

    numBytesRead = recv(client_socket, buffer, MAX, 0);
    if (numBytesRead < 0)
    {
        perror("Failed to read the file name from the client");
        return;
    }

    printf("Attempting to read file %s\n", buffer);
    fd = openFileForRead(buffer);
    memset(buffer, 0, sizeof(buffer));
    if (fd < 0)
    {
        perror("Failed to open the file");
        strncpy(buffer, strerror(errno), MAX);
        if (send(client_socket, buffer, strlen(strerror(errno)), 0) < 0)
        {
            perror("Failed to send the data to the client");
        }
        return;
    }

    printf("Writing file to client\n");
    while((numBytesRead = read(fd, buffer, sizeof(buffer))) > 0) {
        if (send(client_socket, buffer, numBytesRead, 0) < 0)
        {
            perror("Failed to send the data to the client");
            break;
        }
        memset(buffer, 0, sizeof(buffer));
    }

    close(fd);
    return;
}

int openFileForRead(char *filename)
{
    if (NULL == filename)
    {
        return -1;
    }

    return open(filename, O_RDONLY);
}