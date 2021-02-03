#include <stdio.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <string.h> 
#include<fcntl.h>
#define MAX 20000

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
    if (listen(s, 2) < 0){
        close(s);
        puts("Error: listen");
        return -1;
    }
    return s;  
}

int main(int argc, char const *argv[]){ 
    int socket = tcpServerSocket(6666);

    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    int csocket = accept(socket, (struct sockaddr *) &addr, &len);

    int n=0,i=0;;
    char buffer[MAX] = {0};
    int fd = open("hurroBugo.txt",O_WRONLY|O_CREAT|O_APPEND,0666);
    while( (n = recv(csocket, buffer, MAX, 0))>0){
        if(n>0){
            printf("%d\n",n);
            write(fd,buffer,n);
        }
        memset(buffer,0,MAX);

    }
    close(csocket);
    close(fd);
} 
