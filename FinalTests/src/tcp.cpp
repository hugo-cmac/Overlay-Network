
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
    if (listen(s, MAXFDS) < 0){
        close(s);
        puts("Error: listen");
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
    return s;
}