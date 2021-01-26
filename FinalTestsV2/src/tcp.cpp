
#include "tcp.h"

int tcpServerSocket(int port){
    int s = socket(AF_INET, SOCK_STREAM, 0);
    int one = 1;
    if (s < 0){
    	puts("Error: openning Socket descriptor");
        return -1;
    }
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind( s, (struct sockaddr *) &addr, sizeof(addr)) < 0){
    	close(s);
    	puts("Error: port binding");
        return -1;
    }
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) < 0) {
		close(s);
		puts("Error: SO_REUSEADDR");
		return -1;
	}
    if (setsockopt(s, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one)) < 0) {
		close(s);
		puts("Error: SO_REUSEPORT");
		return -1;
	}
    if (listen(s, MAXFDS/4) < 0){
        close(s);
        puts("Error: listen");
        return -1;
    }
    int size = 4194304;
    //////////////////////
    //Set send buffer size
    if(setsockopt(s, SOL_SOCKET, SO_SNDBUF, &size, 4) < 0){
        perror("Error on setting send buffer size");
        return -1;
    }
    size = 6291456;
    //////////////////////
    //Set recv buffer size
    if(setsockopt(s, SOL_SOCKET, SO_RCVBUF, &size, 4) < 0){
        perror("Error on setting recv buffer size");
        return -1;
    }

    return s;  
}

int tcpClientSocket(unsigned int ip, int port){
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0){
    	puts("Error: openning Socket descriptor");
        return -1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = ip;

    if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) < 0){
    	close(s);
        puts("Error: connecting to server");
        return -1;
    }
    int size = 4194304;
    //////////////////////
    //Set send buffer size
    if(setsockopt(s, SOL_SOCKET, SO_SNDBUF, &size, 4) < 0){
        perror("Error on setting send buffer size");
        return -1;
    }
    size = 6291456;
    //////////////////////
    //Set recv buffer size
    if(setsockopt(s, SOL_SOCKET, SO_RCVBUF, &size, 4) < 0){
        perror("Error on setting recv buffer size");
        return -1;
    }

    return s;
}

int tcpClientSocketIPv6(unsigned char* ip, int port){
    int s = socket(AF_INET6, SOCK_STREAM, 0);
    if (s < 0){
    	puts("Error: openning Socket descriptor");
        return -1;
    }

    struct sockaddr_in6 addr;
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(port);
    memcpy(addr.sin6_addr.s6_addr, ip, 16);
    
    inet_pton(AF_INET6, (const char*) ip, &addr.sin6_addr);

    if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) < 0){
    	close(s);
        puts("Error: connecting to server");
        return -1;
    }
    return s;
}