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
#include <time.h>

typedef unsigned char byte;


int main(int argc, char const *argv[]){
    
    
    // unsigned int vector = 0;
    // byte circuit = 0xac;//10 10 11 00
    // byte mask = 0x03;
        
    // int temp=0;
            
    // temp = (circuit>>2) & mask;
    // vector += 1<<(temp<<3);

    // temp = (circuit>>4) & mask;
    // vector += 1<<(temp<<3);
            
    // temp = (circuit>>6) & mask;
    // vector += 1<<(temp<<3);

    // printf("%d\n",vector);

    // //16908288




    //  int dir = UP;

    // byte circuito = 0;

    // circuito = dir<<(hop<<1); //first dir
    // circuito = dir<<(hop<<1); //secon dir
    // circuito = dir<<(hop<<1); //third dir
    // circuito |= hop; //max 2

    // //map< pair<circuito, streamID>, streamID>
    // //map< streamID, pair<circuito, streamID>>
    // //[] [3] [] []
    // //11 10  01 00
       

    // unsigned int vector = 0;
    // byte circuit = 0;
    // byte mask = 0x03;

    // int temp=0;
            
    // temp = (circuit>>2) & mask;
    // vector += 1<<(temp<<3);

    // temp = (circuit>>4) & mask;
    // vector += 1<<(temp<<3);
            
    // temp = (circuit>>6) & mask;
    // vector += 1<<(temp<<3);




    srand( time( NULL));
    for (int i = 0; i < 3; ++i){
        int c = rand() % 3 + 1;
        printf("%d\n", c);
    }
    
    return 0;
}