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
#include <bits/stdc++.h>

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


short availableNeighbor[4] = {0};//var[00]=0/var[01]=1/var[10]=1/var[11]=0
short type;//=2 vertice

struct pollfd localSocks [MAXFDS] = {0};
struct pollfd  exitSocks [MAXFDS] = {0};
struct pollfd  neighbors [4]      = {0};

//      < vector,  streamID >, streamID
map<pair<unsigned int, short>, short> streamIdentifier;

//  streamID,  < vector,  streamID >
map<short, pair<unsigned int, short>> pathIdentifier;

unsigned int path[MAXFDS] = {0};


unsigned int timeR = time(NULL);

//objeto para x509
//objeto para echd
//                          1B          1B            2b            1b                  2b                     2b                       
//objeto para ler pacotes  <tp> <circuito 1byte > <saltos 4> <flag exit/local> <flag new/talk/end> <flag ip/domain/payload> <payload>
//                                  
//procedimento de sockv5/v4
//procedimento de abertura de exitSocks(map para streams)

int random(int n){
    timeR++;
    srand( timeR);
    return rand() % n;
}

void printBinary(byte b){
    short i = 8;
    while(i--){
        if (((b>>i) & 0x01) == 1){
            printf("1 ");
        }else{
            printf("0 ");
        }
    } 
    printf("\n");
}


class Protocol{
    private:
        byte buffer[PACKET] = {0};
        byte mask = 0x03;//0xfc

        //byte circuit = 0;

        short nb = 0;
        byte direction = -1;

        byte getRandomDirection(byte hop){
            int i = random(type);//para tras impossivel

            while(!availableNeighbor[i]){
                if (this->direction == i){
                    i = random(type);
                }
            }
            byte dir = i;
            this->buffer[0] |= (dir & this->mask)<<(hop<<1);
        
            return dir;
        }

        byte getNewPath(byte vector[4]){
            int dir[3];
            int f = 0, s = 0, t = 0;

            while(f < 4){
                if (vector[f]){
                    vector[f] -= 1;
                    dir[s++] = f;
                }else{
                    f++;
                }
            }
            
            f = random(3);
            while(!availableNeighbor[dir[f]]){
                f = random(3);
            }
            printf("f = %d\n", f);

            s = random(3);
            while(s == f){
                s = random(3);
            }
            printf("s = %d\n", s);

            t = 3 - (f + s);

            printf("t = %d\n", t);
            byte newCircuit = dir[f]<<6 | dir[s]<<4 | dir[t]<<2 | 3;

            return newCircuit;
        }

    public:
        Protocol(short dir){
            this->direction = dir; 
        }

        //PACKET RELATED 
        void setPacket(byte* buffer, short n){
            memcpy(&this->buffer[3], buffer, n);
        }

        int get(){
            nb = recv(neighbors[this->direction].fd, this->buffer, PACKET, 0);
            if (nb < 0){
                //algum tipo de erro
            }
            return nb;
        } 
        int forward(){
            nb = send(neighbors[this->direction].fd, this->buffer, PACKET, 0);
            if (nb < 0){
                //algum tipo de erro
            }

            return nb;
        }


        //PACKET PARAMETER RELATED
        short getNextDirection(){
            //   6bit     2bits | 2bits        2bits       |    6bits    |   Resto
            // <circuito> <hop> | <new>  <ip/ipv6/dominio> |  <streamID> | <Payload>

            //                    2bits      2bit(0/1)     |    6bits    |   Resto
            // <circuito> <hop> | <talk>   <local/exit>    |  <streamID> | <Payload>
            // <circuito> <hop> | <end>    <local/exit>    |  <streamID> |

            byte type = (this->buffer[1]>>6) & this->mask;//saber se é new ou nao
            byte hops = (this->buffer[0]) & this->mask;//numero de hops

            if (hops == 0){
                switch(type){
                    case NEW:
                        return 1;
                    case TALK:
                        return 2;
                    case END:
                        return 3;
                }
            }else{
                if (type == NEW){//saber se é new ou nao
                    this->direction = getRandomDirection(hops);
                }else{
                    this->direction = (this->buffer[0]>>(hops<<1)) & mask;
                    //recalc
                }
                hops -= 1;//decrementar saltos
                this->buffer[0] = (this->buffer[0] & 0xfc) | (hops & mask);
            }
            return 0;
        }

        unsigned int getVector(){
            unsigned int vector = 0;    
            int temp=0;
                    
            temp = (this->buffer[0]>>2) & mask;
            vector += 1<<(temp<<3);

            temp = (this->buffer[0]>>4) & mask;
            vector += 1<<(temp<<3);
                    
            temp = (this->buffer[0]>>6) & mask;
            vector += 1<<(temp<<3);

            return vector;
        }

        short getStreamID(){
            short streamID = this->buffer[2] & 0x3f;
            
            return streamID;
        }
        bool isExitNode(){
            if ((this->buffer[1]>>4) & mask) {//se for local
                return true;
            }
            return false;
        }

        byte* getPayload(){
            return &buffer[3];
        }

        //PACKET CREATION
        void build(short streamID, unsigned int vector, bool exit){
            
            byte circuit = getNewPath((byte*) &vector);
            
            printBinary(circuit);

            this->buffer[0] = circuit;
            
            this->buffer[1] = 1<<6;
            
            if (exit)
                this->buffer[1] |= 1<<4;

            this->buffer[2] = streamID & 0x3f;

            getNextDirection();
        }

        void end(short streamID, unsigned int vector, bool exit){
            memset(buffer, 0, PACKET);
            byte circuit = getNewPath((byte*) &vector);
            
            printBinary(circuit);

            this->buffer[0] = circuit;
            
            this->buffer[1] = 2<<6;
            
            if (exit)
                this->buffer[1] |= 1<<4;

            this->buffer[2] = streamID & 0x3f;

            getNextDirection();
        }

};

unsigned int vectorCalc(byte circuit){
    unsigned int vector = 0;    
    int temp=0;
                    
    temp = (circuit>>2) & 0x03;
    vector += 1<<(temp<<3);

    temp = (circuit>>4) & 0x03;
    vector += 1<<(temp<<3);
                    
    temp = (circuit>>6) & 0x03;
    vector += 1<<(temp<<3);

    return vector;
}

unsigned int invert(unsigned int vector){
    
    unsigned int inverse = 0;

    inverse += vector>>16 & 0x03;
    inverse += vector>>24 & 0x03;
    inverse += vector>>0  & 0x03;
    inverse += vector>>8  & 0x03;
    
    return 0;
}

short findEmpty(struct pollfd* list, int fd){
    short i = -1;
    while (i++ < MAXFDS){
        if (list[i].fd == -1){   
            list[i].fd = fd;
            list[i].events = POLLIN;
            return i;
        }
    }
    return 0;
}

void closeLocal(short streamID){

}
void closeExit(pair<unsigned int,short> p){
    
}

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

    if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) < 0){
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
    send(fd, answer, 2, 0);
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
    }
    return 1;
}

int handleClientRequest(int fd){ // 0x01: ipv4 | 0x03: Domain | 0x04: ipv6
	byte command[4];
	int nread = recv(fd, command, 4, 0);
	return command[3];
}

char *readIPV4(int fd){
    char *ipv4 = (char *)malloc(4);
    if(recv(fd, ipv4, 4, 0)<0){
        //erro
    }
    return ipv4;  
}

char *readDomain(int fd, byte *size){
    byte s;
    if(recv(fd, &s, 1, 0) < 0){ // obter tamanho do dominio
        // erro
    }else{
        char *domain = (char *)malloc((int)s + 1);
        if(recv(fd, domain, (int)s, 0) < 0){ // ler dominio
            // erro
        }else{
            domain[s] = '\0';
            *size = s;
            return domain;
        }
    }
    return NULL;
}

char *readIPV6(int fd){
    char *ipv6 = (char *)malloc(16);
    if(recv(fd, ipv6, 16, 0)<0){
        //errod
    }
    return ipv6;
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
            //algum tipo de erro
            close(proxyClient);
        } else {
            //sockv5 connection handler
            ///....



            //se tudo der correto
            findEmpty(localSocks, proxyClient);
            
            thread psvHandler(proxyServerProcedure, proxyClient);
            psvHandler.detach();
            //sq fazer algo
        }
    }
}

void proxyLocalProcedure(byte* buffer, short n, short streamID){
    Protocol packet(-1);
    packet.setPacket(buffer, n);

    packet.build(streamID, path[streamID], true);
    packet.forward();
}

//threadB
void proxyLocalHandler(){
    short streamID, result = -1, n;
    byte* buffer = (byte*)malloc(PACKET);
    
    while (1){
        result = poll(localSocks, MAXFDS, -1);
        if (result < 0){
            //algum tipo de erro
        }
        streamID = -1;
        while (streamID++ < MAXFDS){
            if (localSocks[streamID].revents & POLLIN){
                n = recv(localSocks[streamID].fd, buffer, PACKET, 0);

                thread slave(proxyLocalProcedure, buffer, n, streamID);
                slave.detach();
                
                localSocks[streamID].revents = 0;
                break;
            }
        }
    }
}

int webConnect(){
    return 0;
}

void forwardingProcedure(short dir){
    
    short streamID = 0;
    if (dir < 4){//pacote da rede overlay

        Protocol packet(dir);
        packet.get();
        
        dir = packet.getNextDirection();//caso seja para encaminhar faz aqui a alteracao do salto

        streamID = packet.getStreamID();

        switch(dir){
            case 0://é para encaminhar
                packet.forward();
                break; 
            case 1:{//new exitSocket
                pair<unsigned int,short> p(packet.getVector(), streamID);
                pair<unsigned int,short> r(invert(packet.getVector()), streamID);

                int webSocket = webConnect();
                if (webSocket > 0){
                    short s = findEmpty(exitSocks, webSocket);
                    streamIdentifier[p] = s;
                    pathIdentifier[s] = r;
                }else{
                    packet.end(streamID, r.first, false);
                    packet.forward();
                }
                break;
            }
            case 2://talk
                if (packet.isExitNode()){
                    pair<unsigned int,short> p(packet.getVector(), streamID);

                    send(exitSocks[streamIdentifier[p]].fd, NULL, PACKET, 0);
                }else{
                    if (path[streamID] == 0)
                        path[streamID] = invert(packet.getVector());
                    send(localSocks[streamID].fd, NULL, PACKET, 0);
                }
                break;
            case 3://end
                if (packet.isExitNode()){
                    pair<unsigned int,short> p(packet.getVector(), streamID);
                    //pair<unsigned int, short> p = {}
                    //streamIdentifier[{}];
                    //pathIdentifier
                    //pair 
                    closeExit(p);
                }else{
                    closeLocal(streamID);
                }
                break;
        }        
    } else {//manager
        //novo vizinho( dir, ip )
        //estabelecer ligacao com o novo nó
        //avisar qual a dir do novo vizinho
    }
}

//threadC
void forwardingHandler(int manager, int type){
    

    short result = -1, streamID = -1, dir;

    byte* buffer = (byte*)malloc(PACKET);
    

    //poll atributes
    while (1){

        result = poll(neighbors, 4, -1);

        if (result < 0){
            //algum tipo de erro
        }

        dir = streamID = -1;

        while (dir++ < 5){

            if (neighbors[dir].revents & POLLIN){
                thread slave (forwardingProcedure, dir);
                slave.detach();
                //pthread_create(&slave, NULL, forwardingProcedure,(void*)dir);
                
                neighbors[dir].revents = 0;
            }
            memset(buffer, 0, PACKET); 
        }
    }
}

//threadD
void exitNodeHandler(){
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
                
                exitSocks[streamID].revents = 0;
                break;
            }
        }
    }
}


int main(int argc, char const *argv[]){
    
    type = 2;
    availableNeighbor[1] = 1;
    availableNeighbor[2] = 1;
       
    Protocol packet(-1);
    byte buffer[1400] = {0};

    packet.setPacket(buffer, 1400);
    printBinary(0xf4);
    unsigned int v = vectorCalc(0xf4);
    packet.build(1, v, true); // 8 4 2 1 8 4 2 1
                              // 1 1 1 1 0 1 0 0
                              // 0xf4

    //1 - abrir server socket 
    //2 - ligar ao manager, atraves de ip pre definido
   
    //3 - mandar id, cert -> quer me ligar a rede
    //4 - em principio devera receber uma reposta //recv() -> tipo de no
    //5 - accept() >> recv() dir
    //6 - recv() dir

    //neighbors[dir].fd = nova socket;
    //  msg vinda do manager ip/dir
    //  o meu vizinho da direito bazou




    // 0 - 0 - 0 - x
    // 0 - 0 - 0 -
    // 0 - 0 - 0 -
    // |   |   |

    return 0;
}
