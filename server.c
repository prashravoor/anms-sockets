#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "common.h"
#include <signal.h>
#include <sys/time.h>

#define DEFAULT_QUEUE_SIZE 5
#define DEFAULT_FORK_POOL_SIZE 100
#define ENV_NO_FORK_CHILDREN "NO_FORK_CHILDREN"
#define MAX 1000

extern int errno;

int startServer(int port, int maxQueueSize);
void serveFile(int client_socket);
int openFileForRead(char *name);
void stopServer(int signum);

static int server_socket, numClientsServiced;

int main(int argc, char **argv)
{
    int port = DEFAULT_SERVER_PORT, maxQueueSize = DEFAULT_QUEUE_SIZE;
    int client_socket, fork_children = 1, child_pid;
    struct sockaddr_in client_address;
    socklen_t client_len = 0;
    struct sigaction handler;

    struct timeval startTime, endTime;

    // Usage: rcatserver <Optional Port> <Optional Max Queue Size>
    if (argc != 1 && argc != 3)
    {
        printf("Invalid command syntax\n\nUsage: %s <Optional Port> <Optional Max Queue Size>\n", argv[0]);
        return 1;
    }

    if (3 == argc)
    {
        port = atoi(argv[1]);
        if (port < 1024)
        {
            printf("Port number is not valid, specify a port value >= 1024\n");
            return 1;
        }

        maxQueueSize = atoi(argv[2]);
        if (maxQueueSize < 0 || maxQueueSize > 5)
        {
            printf("Invalid value for max queue size, enter a value between 0 and 5\n");
            return 1;
        }
    }

    // Export NO_FORK_CHILDREN in the bash to set the server to iterative processing mode
    // Default mode is concurrent
    if (getenv(ENV_NO_FORK_CHILDREN))
    {
        fork_children = 0;
    }

    /*
    * Signal Handling. If the sever is terminated through Ctrl+C or through SIGTERM,
    * then the handles of the server need to be closed. Add a singal handler for both signals.
    * This process forks several children, none of which get removed from the kernel automatically, since 
    * all child processes are tied to the execution of the parent.
    * Set SIGCHLD signal handler to Ignore, to auto cleanup all completed child processes.
    */
    memset(&handler, 0, sizeof(handler));
    handler.sa_handler = stopServer;
    sigaction(SIGTERM, &handler, NULL);
    sigaction(SIGINT, &handler, NULL);
    signal(SIGCHLD, SIG_IGN);

    server_socket = startServer(port, maxQueueSize);
    if (server_socket < 0)
    {
        error("Failed to start the server", -1);
    }

    while (1)
    {
        //printf("Server is now listening for connections\n");
        memset(&client_address, 0, sizeof(client_address));
        client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_len);
        gettimeofday(&startTime, NULL);
        //printf("Connection received: %d\n", client_socket);
        if (client_socket < 0)
        {
            perror("Failed to accept the connection");
            continue;
        }

        // If the server is running in concurrent mode, then fork a child process to
        // service the received connection. The parent goes back to listening for connections
        if (fork_children && (child_pid = fork()) == 0)
        {
            serveFile(client_socket);
            close(client_socket);
            close(server_socket);
            gettimeofday(&endTime, NULL);
            printf("Child: Time to service client socket %d was %lumicros\n", client_socket, diffTime(&endTime, &startTime));
            return 0;
        }
        else if (!fork_children || child_pid < 0)
        {
            // Either iterative mode server or forking a child failed.
            serveFile(client_socket);
        }

        ++numClientsServiced;
        close(client_socket);
        gettimeofday(&endTime, NULL);
        printf("Total time to service client: %lumicros\n", diffTime(&endTime, &startTime));
    }

    close(server_socket);
    return 0;
}

/**
 * Start the server on all local IP addresses, with the specified port.
 */
int startServer(int port, int maxQueueSize)
{
    struct sockaddr_in server_address;

    server_socket = socket(SOCKET_DOMAIN, SOCKET_TYPE, 0);
    if (server_socket < 0)
    {
        error("Failed to create the socket for the server", server_socket);
    }
    //printf("Succesfully created socket: %d\n", server_socket);

    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = SOCKET_DOMAIN;
    server_address.sin_addr.s_addr = INADDR_ANY;
    // htons converts the host specific byte ordering for the port number to a network specific ordering
    server_address.sin_port = htons(port);

    if (bind(server_socket, (const struct sockaddr *)&server_address, sizeof(server_address)))
    {
        error("Failed to bind the socket to the server", server_socket);
    }

    //printf("The server is bound to port %d\n", port);
    if (listen(server_socket, maxQueueSize))
    {
        error("Listening for connections interrupted", server_socket);
    }
    return server_socket;
}

void stopServer(int signum)
{
    printf("Handled Signal %d\n", signum);
    printf("Total serviced clients: %d\n", numClientsServiced);
    close(server_socket);
    exit(0);
}

/**
 * The main function of the server. Reads the specified filename,
 * and writes the contents of the file to the client.
 */
void serveFile(int client_socket)
{
    char buffer[MAX], *error = NULL;
    int numBytesRead = 0, fd = 0;
    //printf("Attempting to read file name from socket %d\n", client_socket);
    memset(buffer, 0, sizeof(buffer));

    numBytesRead = recv(client_socket, buffer, MAX, 0);
    if (numBytesRead < 0)
    {
        perror("Failed to read the file name from the client");
        return;
    }

    //printf("Attempting to read file %s\n", buffer);
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

    //printf("Writing file to client\n");
    while ((numBytesRead = read(fd, buffer, sizeof(buffer))) > 0)
    {
        if (send(client_socket, buffer, numBytesRead, MSG_NOSIGNAL) < 0)
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
