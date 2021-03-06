#ifndef COMMON_H
#define COMMON_H

#define SOCKET_DOMAIN AF_INET
#define SOCKET_TYPE SOCK_STREAM
#define DEFAULT_SERVER_PORT 8080
#include <netinet/in.h>
#include <sys/time.h>

void error(char* msg, int socket);

int getIpDetails(char *ipAddress, int port, struct sockaddr_in* sockaddr);

unsigned long diffTime(struct timeval* endTime, struct timeval* startTime);

#endif
