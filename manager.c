#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

enum vertical {
    UP   = 0x00,
    DOWN = 0x02
};

enum horizontal {
    LEFT  = 0x01,
    RIGHT = 0x03
};

typedef struct Client {
    uint16_t nodeid;
}client;


client *network = NULL;

uint16_t side  = 2;
uint16_t nodes = 0;

int tcpSocket(int family, int port){
    
    int s = socket(family, SOCK_STREAM, 0);
    if (s < 0){
        perror("Error on openning Socket descriptor");
        return -1;
    }
    struct sockaddr_in addr = {family, htons(port)};
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind( s, (struct sockaddr *) &addr, sizeof(addr)) < 0){
        perror("Error on port binding");
        return -1;
    }
    
    if (listen(s, 8) < 0){
        perror("listen");
        return -1;
    }

    return s;
}

//int getDomainInfo(){
//    getaddrinfo()
//}

int newClient(client network[side][side]){

}
int fillmatrix(int s, int n[s][s]){
    for (size_t i = 0; i < s; i++){
        for (size_t f = 0; f < s; f++){
            n[i][f] = i+f;
            printf("%d   ", n[i][f]);
        }
        printf("\n");
    }
    return 0;
}

void printMatrix(int s, int n[s][s]){

    for (size_t i = 0; i < s; i++){
        for (size_t f = 0; f < s; f++){
            printf("%d   ", n[i][f]);
        }
        printf("\n");
    }
    
}

void rearrange(int s, int n[s][s]){
    for (int i = s-1; i >= 0; i--){
        for (int f = s-1; f >= 0; f--){
            printf("%d   ", n[i][f]);
        }
        printf("\n");
    }
}
int main(int argc, char const *argv[]){
    
    network = malloc((side*side)*sizeof(client));
    

    int *n = malloc(4*sizeof(int));
    fillmatrix(2, n);
    printf("\n");
    n = realloc( n, 9*sizeof(int));
    printMatrix(3, n);
    printf("\n");
    rearrange(3, n);


    return 0;
}