#ifndef TCP_H_
#define TCP_H_

#include <cstdio>

#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#define MAXFDS  128

int tcpServerSocket(int port);

int tcpClientSocket(unsigned int ip, int port);

int tcpClientSocketIPv6(unsigned char* ip, int port);

#endif