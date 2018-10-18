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

