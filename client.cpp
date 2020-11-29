//c++ libs
#include <iostream>
#include <map>
#include <stack>
#include <string>
#include <iterator>
#include <cstring>
#include <thread>

//c libs
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include <pthread.h>

typedef unsigned char byte;

using namespace std;

#define MAXFDS 64
#define PACKET 1500

enum DIRECTION {
    UP = 0,
    RIGHT,
    DOWN,
    LEFT
};

enum CONTENTTYPE{
    NEW = 0,
    TALK,
    END  
};

short availableNeighbor[4];
short type;

struct pollfd localSocks [MAXFDS] = {0};
struct pollfd  exitSocks [MAXFDS] = {0};
struct pollfd  neighbors [4]      = {0};


//objeto para x509
//objeto para echd
//objeto para pipes         1B          1B            2b            1b                  2b                     2b                       
//objeto para ler pacotes  <tp> <circuito 1byte > <saltos 4> <flag exit/local> <flag new/talk/end> <flag ip/domain/payload> <payload>
//                                  
//procedimento de sockv5/v4
//procedimento de abertura de exitSocks(map para streams)


class Protocol{
    private:
        byte buffer[PACKET] = {0};
        short nb = 0;
        short direction = -1;

        short getRandomDirection(){
            //i = (random de type)
            //dir = availableNeighbor[i]
            //adicionar dir a buffer
            //decrementar saltos
            return 0;
        }

    public:
        Protocol(long dir){
            this->direction = (short)dir;
        }

        ~Protocol();

        int get(){
            nb = recv(neighbors[this->direction].fd, this->buffer, PACKET, 0);
            if (nb < 0){
                //algum tipo de erro
            }
            return nb;
        } 
        int send(){
            nb = send(neighbors[this->direction].fd, this->buffer, PACKET, 0);
            if (nb < 0){
                //algum tipo de erro
            }


            return nb;
        }

        short getNextDirection(){
            //  1byte    2bits  2bits       2bits         6bits      Resto
            // <circuit> <new>  <hop> <ip/ipv6/dominio> <streamID> <Payload>
            // <circuit> <talk> <hop>   <local/exit>    <streamID> <Payload>
            // <circuit> <end>  <hop>   <exit/local>    <streamID>

            short type = this->buffer[1];//saber se é new ou nao
            
            if (type == NEW){
                short newDir = getRandomDirection();
                return newDir;
            }else{
                this->direction = this->buffer[]
            }

            return 0;
        }

    
};



int tcpServerSocket(int port){
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0){
        perror("Error on openning Socket descriptor");
        return -1;
    }
    struct sockaddr_in addr = {AF_INET, htons(port)};
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind( s, (struct sockaddr *) &addr, sizeof(addr)) < 0){
        perror("Error on port binding");
        return -1;
    }
    if (listen(s, 10) < 0){
        perror("listen");
        return -1;
    }
    return s;
}

int tcpClientSocket(uint32_t ip, int port){
    int s = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr = {AF_INET, htons(port)};
    addr.sin_addr.s_addr = ip;
    //inet_pton(AF_INET, ip, &addr.sin_addr); para char*

    if (connect(s, (struct sockaddr *)&addr,sizeof(addr)) < 0){
        perror("Error connecting to server");
        return -1;
    }

    return s;
}

void greetingsAnswer(int fd,  int auth_method){
    byte answer[2];
    answer[0] = 0x05;
    if(auth_method){ // No method accepted
        answer[1] = 0xff;
    }else{ // Accept no_auth
        answer[1] = 0x00;
    }
    send(fd, answer, 2, NULL);
}

int clientGreetings(int fd, int *version){
    byte auths[64];
    int nread = recv(fd, auths, 64, 0);
    if(nread < 0){
        // erro
    }else{
        if(auths[0] == 0x05){
            if(auths[1] < 1){
                // nao suporta nenhum tipo de autenticação
                greetingsAnswer(fd, 1);
            }else{
                for(int i=2 ; i<nread; i++){
                    if(auths[i] == 0x00){
                        greetingsAnswer(fd, 0);
                        return 0;
                        break;
                    }
                }
            }
        }
        greetingsAnswer(fd, 1);
        return 1;
    }

}

int handleClientRequest(int fd){ // 0x01: ipv4 | 0x03: Domain | 0x04: ipv6
	byte command[4];
	int nread = recv(fd, command, 4, NULL);
	return command[3];
}

char *readIPV4(int fd){
    char *ipv4 = (char *)malloc(sizeof(char) * 4);
    if(recv(fd, ipv4, 4, NULL)<0){
        //erro
    }else{
        return ipv4;
    }
}

char *readDomain(int fd, byte *size){
    byte s;
    if(recv(fd, (void *)&s, 1, 0)<0){ // obter tamanho do dominio
        // erro
    }else{
        char *domain = (char *)malloc((sizeof(char) * s) + 1);
        if(recv(fd, (void*)domain, (int)s, NULL)<0){ // ler dominio
            // erro
        }else{
            domain[s] = 0;
            *size = s;
            return domain;
        }
    }
}

char *readIPV6(int fd){
    char *ipv6 = (char *)malloc(sizeof(char) * 16);
    if(recv(fd, ipv6, 16, NULL)<0){
        //erro
    }else{
        return ipv6;
    }
}

void proxyServerProcedure(int proxyClient){
    int version = 0;
    if(clientGreetings(proxyClient, &version)){ // Receber primeiro pacote e responder
        // deu erro
    }
    int tipo = handleClientRequest(proxyClient);
    if(tipo == 0x01){ // IPV4
        char *ipv4 = readIPV4(proxyClient);
    }
    if(tipo == 0x03){ // Domain
        byte size;
        char *address = readDomain(proxyClient, &size);
    }
    if(tipo == 0x04){ // IPV6
        char *ipv6 = readIPV6(proxyClient);
    }

    short i = -1;
    while (i++ < MAXFDS){
        if (localSocks[i].fd == -1){
            localSocks[i].fd = proxyClient;
            localSocks[i].events = POLLIN;
            break;
        }
    }
}

//threadA
void proxyServerHandler(){
    int proxyServer = tcpServerSocket(9999);
    while (1){
        struct sockaddr_in client = {0};
        socklen_t len = sizeof(struct sockaddr_in);
        int proxyClient = accept(proxyServer, (struct sockaddr*) &client, (socklen_t *)&len);
        if (proxyClient < 0){
            close(proxyClient);
        } else {
            //sockv5 connection handler
            ///....
            thread psvHandler(proxyServerProcedure, proxyClient);
            psvHandler.detach();
            //sq fazer algo
        }
    }
}

//threadB
void proxyLocalHandler(int pipe){
    short streamID, result = -1;
    byte* buffer = (byte*)malloc(PACKET);
    
    while (1){
        result = poll(localSocks, MAXFDS, -1);
        if (result < 0){
            //algum tipo de erro
        }
        streamID = -1;
        while (streamID++ < MAXFDS){
            if (localSocks[streamID].revents & POLLIN){
                recv(localSocks[streamID].fd, buffer, PACKET, 0);
                write(pipe, &streamID, 2);
                write(pipe, buffer, PACKET);
                localSocks[streamID].revents = 0;
                break;
            }
        }
    }
}

void *forwardingProcedure(void *d){
    long dir;
    dir = (long)d;

    Protocol packet(dir);

    packet.get();

    if (dir < 4){//pacote da rede overlay
        
        dir = packet.getNextDirection()//caso seja para encaminhar faz aqui a alteracao do salto

            if (dir < 0) { //é para aqui

                streamID = isExitNode(buffer)// devolve streamID
                //adicionar aqui flag de fecho de descritor

                if (streamID < 0) { // exit sock

                    send(exitSocks[-streamID].fd, buffer)

                } else { // local sock

                    send(localSocks[streamID].fd, buffer)

                }

            } else {//é para encaminhar
                packet.send();
            }
        
    } else {//manager
        //novo vizinho( dir, ip )
        //estabelecer ligacao com o novo nó
        //avisar qual a dir do novo vizinho
    }






    pthread_exit(NULL);
}

//threadC
void forwardingHandler(int manager, int pipe, int type){
    map<short, byte> stream;

    short result = -1, streamID = -1, dir;

    byte* buffer = (byte*)malloc(PACKET);
    
    pthread_t slave;

    ///////////////////
    // pthread_create(&threads[i], NULL, PrintHello, (void *)i);
    //
    ///////////////////

    //poll atributes
    while (1){

        result = poll(neighbors, 4, -1);

        if (result < 0){
            //algum tipo de erro
        }

        dir = streamID = -1;

        while (dir++ < 6){

            if (neighbor[dir].revents & POLLIN){

                if (dir < 5){//protocolo
                    pthread_create(&slave, NULL, forwardingProcedure,(void*)dir);
                    

                    
                } else {// pipe / local proxy / exit proxy
                    
                    read(pipe, &streamID, 2);//
                    
                    if (/*new circuit*/){
                        // construir novo pacote do protocolo
                    } else {
                        // construir pacote do protocolo
                    }
                    // encriptar
                    // enviar
                    


                }
                neighbor[dir].revents = 0;
            }
            memset(buffer, 0, PACKET); 
        }
    }
}

//threadD
void exitNodeHandler(int pipe){
    short streamID, result = -1;
    byte* buffer = (byte*)malloc(PACKET);
    
    while (1){
        result = poll(exitSocks, MAXFDS, -1);
        if (result < 0){
            //algum tipo de erro
        }
        streamID = -1;
        while (streamID++ < MAXFDS){
            if (exitSocks[streamID].revents & POLLIN){
                recv(exitSocks[streamID].fd, buffer, PACKET, 0);
                write(pipe, &streamID, 2);
                write(pipe, buffer, PACKET);
                exitSocks[streamID].revents = 0;
                break;
            }
        }
    }
}


int main(int argc, char const *argv[]){
    /* code */



    return 0;
}
