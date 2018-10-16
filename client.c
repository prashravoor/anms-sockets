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
#define MAX_RETRIES 3
#define SLEEP_INTERVAL_MICROS 100000 // 100ms

extern int errno;

void error(char *msg, int socket)
{
    perror(msg);
    if(socket > 0) {
        close(socket);
    }
    exit(1);
}

int main(int argc, char **argv)
{
    int client_socket, port = DEFAULT_SERVER_PORT;
    struct sockaddr_in server_address;
    int numBytesRead, retryCount = 0, connectCode = -1;
    char buffer[MAX], *serverIp = NULL;

    if(argc != 2 && argc != 4) {
        printf("Invalid arguments\n\nUsage: %s <filename> <Optional Server IP> <Optional Server port>\n", argv[0]);
        return 1;
    }

    if(4 == argc) {
        serverIp = argv[2];
        port = atoi(argv[3]);
        if(port < 1024) {
            printf("Port number is not valid, specify a port value >= 1024\n");
            return 1;
        }
    }

    if(strnlen(argv[1], MAX) >= MAX) {
        printf("The filename provided is too long\n");
        return 1;
    }

    client_socket = socket(SOCKET_DOMAIN, SOCKET_TYPE, 0);
    if (client_socket < 0)
    {
        error("Failed to create the socket for the client", client_socket);
    }
    printf("Succesfully created socket: %d\n", client_socket);

    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = SOCKET_DOMAIN;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port);

    printf("Attempting to connect to server on port %d\n", port);
    while (retryCount++ < MAX_RETRIES && connectCode < 0) {
        connectCode = connect(client_socket, (const struct sockaddr *)&server_address, sizeof(server_address));
        if(0 == connectCode || errno != ETIMEDOUT) {
            break;
        }
        perror("Failed to connect to the server");
        printf("Retrying after %d ms\n", (SLEEP_INTERVAL_MICROS/1000));
        usleep(SLEEP_INTERVAL_MICROS);
    }
    if(connectCode < 0) {
        error("Failed to connect to the server", client_socket);
    }

    printf("Successfully connected to server, Sending the File name\n");
    memset(buffer, 0, sizeof(buffer));
    strncpy(buffer, argv[1], strnlen(argv[1], MAX));

    if (send(client_socket, buffer, strnlen(argv[1], MAX), 0) < 0)
    {
        error("Failed to write the data to the server", client_socket);
    }

    memset(buffer, 0, sizeof(buffer));
    printf("Attempting to read contents of file\n");
    while ((numBytesRead = recv(client_socket, buffer, MAX, 0)) > 0)
    {
        printf("%s", buffer);
        memset(buffer, 0, sizeof(buffer));
    }

    printf("\n");
    if(numBytesRead < 0) {
        error("Failed to read contents of the file", client_socket);
    }

    close(client_socket);
    return 0;
}