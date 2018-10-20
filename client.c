#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include <sys/time.h>
#include <signal.h>
#include <sys/wait.h>

#define MAX 4096
#define MAX_RETRIES 3
#define SLEEP_INTERVAL_MICROS 100000 // 100ms
#define ENV_NUM_OF_CLIENTS "NUM_OF_CLIENTS"

extern int errno;

int readRemoteFile(char *fileName, char *serverIp, int port);
void stopClient(int signum);
void childStopped(int);

static int client_socket;

int main(int argc, char **argv)
{
    char *serverIp = NULL;
    int port = DEFAULT_SERVER_PORT, numOfClients = 0, maxNumOfClients = 1;
    int numPassed = 0, numFailed = 0, status, i = 0;
    struct timeval startTime, endTime;
    struct sigaction handler;
    int *child_pids = NULL, childIndex = 0;

    if (argc != 2 && argc != 4)
    {
        printf("Invalid arguments\n\nUsage: %s <filename> <Optional Server IP> <Optional Server port>\n", argv[0]);
        return 1;
    }

    if (4 == argc)
    {
        serverIp = argv[2];
        port = atoi(argv[3]);
        if (port < 1024)
        {
            printf("Port number is not valid, specify a port value >= 1024\n");
            return 1;
        }
    }

    if (strnlen(argv[1], MAX) >= MAX)
    {
        printf("The filename provided is too long\n");
        return 1;
    }

    /**
     * Read the max number of clients this process will fork
     */
    if (getenv(ENV_NUM_OF_CLIENTS))
    {
        maxNumOfClients = atoi(getenv(ENV_NUM_OF_CLIENTS));
        if (maxNumOfClients <= 0)
        {
            maxNumOfClients = 1;
        }
    }
    child_pids = calloc(maxNumOfClients, sizeof(int));

    /*
    * Signal Handling. If the client is terminated through Ctrl+C or through SIGTERM,
    * then the handles of the client need to be closed. Add a singal handler for both signals.
    * This process can fork several children, none of which get removed from the kernel automatically, since 
    * all child processes are tied to the execution of the parent.
    * Set SIGCHLD signal handler to Ignore, to auto cleanup all completed child processes.
    */
    memset(&handler, 0, sizeof(handler));
    handler.sa_handler = stopClient;
    sigaction(SIGTERM, &handler, NULL);
    sigaction(SIGINT, &handler, NULL);
    handler.sa_handler = childStopped;
    sigaction(SIGCHLD, &handler, NULL);

    do
    {
        if ((child_pids[childIndex++] = fork()) == 0)
        {
            free(child_pids);
            gettimeofday(&startTime, NULL);
            readRemoteFile(argv[1], serverIp, port);
            gettimeofday(&endTime, NULL);

            printf("Total time to read the file from the server: %lu micros\n", diffTime(&endTime, &startTime));
            return 0;
        }
    } while (++numOfClients < maxNumOfClients);

    for (i = 0; i < maxNumOfClients; ++i)
    {
        wait(&status);
        if (status)
        {
            ++numFailed;
        }
        else
        {
            ++numPassed;
        }
    }

    printf("Out of %d clients, %d passed, and %d have failed\n", maxNumOfClients, numPassed, numFailed);
    free(child_pids);
    return 0;
}

/**
 * Connects to the remote server, sends the file name to read, and receives
 * the contents of the file, or errors if any, and echo's it back to the user.
 */
int readRemoteFile(char *fileName, char *serverIp, int port)
{
    int numBytesRead, retryCount = 0, connectCode = -1;
    char buffer[MAX];
    struct sockaddr_in server_address;

    memset(&server_address, 0, sizeof(server_address));
    if (getIpDetails(serverIp, port, &server_address))
    {
        printf("Failed to find the IP address to connect to");
        return 1;
    }

    client_socket = socket(SOCKET_DOMAIN, SOCKET_TYPE, 0);
    if (client_socket < 0)
    {
        error("Failed to create the client socket", -1);
    }
    //printf("Succesfully created client socket %d\n", client_socket);

    //printf("Attempting to connect to server on port %d\n", port);
    while (retryCount++ < MAX_RETRIES && connectCode < 0)
    {
        connectCode = connect(client_socket, (const struct sockaddr *)&server_address, sizeof(server_address));
        if (0 == connectCode || errno != ETIMEDOUT)
        {
            break;
        }
        perror("Failed to connect to the server");
        printf("Retrying after %d ms\n", (SLEEP_INTERVAL_MICROS / 1000));
        usleep(SLEEP_INTERVAL_MICROS);
    }
    if (connectCode != 0)
    {
        error("Failed to connect to the server", client_socket);
    }

    //printf("Successfully connected to server, Sending the File name\n");
    memset(buffer, 0, sizeof(buffer));
    strncpy(buffer, fileName, strnlen(fileName, MAX));

    if (send(client_socket, buffer, strnlen(fileName, MAX), MSG_NOSIGNAL) < 0)
    {
        error("Failed to write the data to the server", client_socket);
    }

    memset(buffer, 0, sizeof(buffer));
    //printf("Attempting to read contents of file\n");
    while ((numBytesRead = recv(client_socket, buffer, MAX, 0)) > 0)
    {
        printf("%s", buffer);
        memset(buffer, 0, sizeof(buffer));
    }

    printf("\n");
    if (numBytesRead < 0 && errno != 0)
    {
        error("Failed to read contents of the file", client_socket);
    }

    close(client_socket);
    return 0;
}

void stopClient(int signum)
{
    printf("Handled Signal %d\n", signum);
    close(client_socket);
    exit(0);
}

void childStopped(int signum)
{
    int pid, status;
    wait(&status);
}
