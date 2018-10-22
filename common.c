#include "common.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h> 
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>

void error(char *msg, int socket)
{
    perror(msg);
    if (socket > 0)
    {
        close(socket);
    }
    exit(1);
}

/**
 * Get the socket address for the specified IP and port.
 * If IP is NULL, it implies all local addresses.
 * The ipAddress can be either a hostname or a static IP.
 */
int getIpDetails(char *ip, int port, struct sockaddr_in* sockaddr)
{
	int retCode;
	struct addrinfo hints, *results = NULL;
	char portStr[10];
	if(NULL == sockaddr) {
		return -1;
	}

	memset(&portStr, 0, sizeof(portStr));
	snprintf((char *)&portStr, 10, "%d", port);

    sockaddr->sin_family = SOCKET_DOMAIN;
	if(NULL == ip) {
	    sockaddr->sin_addr.s_addr = INADDR_ANY;
    	// htons converts the host specific byte ordering for the port number to a network specific ordering
		sockaddr->sin_port = htons(port);
		return 0;
	}

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = SOCKET_DOMAIN;
	hints.ai_socktype = SOCKET_TYPE;

	retCode = getaddrinfo(ip, portStr, &hints, &results);
	if(retCode < 0) {
		return retCode;
	}

	sockaddr = memcpy(sockaddr, results[0].ai_addr, sizeof(results[0].ai_addr));
	freeaddrinfo(results);
	return 0;
}

unsigned long diffTime(struct timeval* endTime, struct timeval* startTime)
{
	unsigned long timeMicros = 0;
	if(NULL == startTime || NULL == endTime)
	{
		return 0;
	}

	// Find difference in seconds first
	timeMicros = (endTime->tv_sec - startTime->tv_sec) * 1000000;

	// Add the micros difference
	timeMicros += (endTime->tv_usec - startTime->tv_usec);

	return timeMicros;
}