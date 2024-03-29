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

#define MAXFDS 32
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


short availableNeighbor[4] = {0};
short type;
struct pollfd  neighbors [5] = {0}; 





//////////////////////////////////////////////////////////////////////////////////////
// Exit Sockets                                                                     //
struct pollfd  exitSocks [MAXFDS] = {0};                                            //
// < < vector, streamID >, streamID>                    //////  //  //  // ///////  //
map<pair<unsigned int, short>, short> streamIdentifier; //       ////   //   //     //
                                                        /////     //    //   //     //
// < streamID, < vector, streamID >                     //       ////   //   //     //
map<short, pair<unsigned int, short>> pathIdentifier;   //////  //  //  //   //     //
//////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////
                                                                                    //
// Path to exit node                            //     ////// //////  ////  //      //
unsigned int path[MAXFDS] = {0};                //     //  // //     //  // //      //
                                                //     //  // //     ////// //      //
// Local Sockets                                //     //  // //     //  // //      //
struct pollfd localSocks [MAXFDS] = {0};        ////// ////// ////// //  // //////  //
//////////////////////////////////////////////////////////////////////////////////////



unsigned int timeR = time(NULL);

//objeto para x509
//objeto para echd

//procedimento de abertura de exitSocks(map para streams)

int random(int n){
    timeR++;
    srand( timeR);
    return rand() % n;
}

void printBinary(byte b){
    short i = 8;
    while(i--){
        if (((b>>i) & 0x01) == 1)
            printf("1 ");
        else
            printf("0 ");
    } 
    printf("\n");
}

class ManagerHandler{
    private:
        uint32_t managerIP;
        int managerPort;
        int dir;
        byte packet[PACKET] = {0};

    public:
        ManagerHandler(uint32_t ip, int port){
            this->managerIP = ip;
            this->managerPort;
            this->dir = 4;
            neighbors[this->dir].fd = tcpClientSocket(this->managerIP, this->managerPort);
        }

        byte* getPacket(){
            return this->packet;
        }
        
        int getNodeTypeDir(){
            return((this->packet[0] && 0xc)>>2);
        }

        int getPacketType(){
            return (this->packet[0] && 0x03);
        }

        void buildRegisterPacket(){
            this->packet[0] = 0;
            this->packet[1] = 1; //certificado
          
        }

        void buildNeighborPacket(int dir){
            this->packet[0] = 3;
            this->packet[1] = this->invertDir(dir);
        }

        void sendManager(){
            if(send(neighbors[this->dir].fd, this->packet, sizeof(this->packet), 0)){
                memset(this->packet, 0, sizeof(this->packet));
            }else{
                //erro
            }
        }

        void sendNeighbor(int dir){
            if(send(neighbors[dir].fd, this->packet, sizeof(this->packet), 0)){
                memset(this->packet, 0, sizeof(this->packet));
            }else{
                //erro
            }
        }

        void get(){
            memset(this->packet, 0, sizeof(this->packet));
            if(recv(neighbors[this->dir].fd, this->packet, sizeof(this->packet), 0)){
                
            }else{
                //erro
            }
        }


        int invertDir(int dir){
            switch (dir){
                case UP:
                    return DOWN;
                    break;

                case DOWN:
                    return UP;
                    break;

                case RIGHT:
                    return LEFT;
                    break;

                case LEFT:
                    return RIGHT;
                    break;
            }
        }
};

class Protocol{
    private:
        byte buffer[PACKET] = {0};
        
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
            this->buffer[0] |= (dir & 0x03)<<(hop<<1);
        
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

            s = random(3);
            while(s == f){
                s = random(3);
            }

            t = 3 - (f + s);

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
            //   6bit     2bits | 2bits     1bits(0/1)      5bits    |   Resto
            // <circuito> <hop> | <new>    <ip/dominio>   <streamID> | <Payload>

            //                    2bits      1bit(0/1)      5bits    |   Resto
            // <circuito> <hop> | <talk>   <local/exit>   <streamID> | <Payload>
            // <circuito> <hop> | <end>    <local/exit>   <streamID> |

            byte type = (buffer[1]>>6) & 0x03;//saber se é new ou nao
            byte hops = (buffer[0]) & 0x03;//numero de hops

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
                    direction = getRandomDirection(hops);
                }else{
                    direction = (buffer[0]>>(hops<<1)) & 0x03;
                    //recalc
                }
                hops -= 1;//decrementar saltos
                buffer[0] = (buffer[0] & 0xfc) | (hops & 0x03);
            }
            return 0;
        }

        unsigned int getVector(){
            unsigned int vector = 0;    
            int temp=0;
                    
            temp = (this->buffer[0]>>2) & 0x03;
            vector += 1<<(temp<<3);

            temp = (this->buffer[0]>>4) & 0x03;
            vector += 1<<(temp<<3);
                    
            temp = (this->buffer[0]>>6) & 0x03;
            vector += 1<<(temp<<3);

            return vector;
        }

        short getStreamID(){  
            return this->buffer[1] & 0x1f;
        }
        bool isExitNode(){
            if ((this->buffer[1]>>5) & 0x01) {
                return true;
            }
            return false;
        }

        byte* getPayload(){
            return &buffer[3];
        }

        //PACKET CREATION
        void freshBuild(bool domain, short streamID, byte* payload, int psize){
            direction = getRandomDirection(3);
            buffer[0] |= 2 & 0x03;

            buffer[1] = 1<<6;
            if (domain)
                this->buffer[1] |= 1<<5;

            this->buffer[1] = streamID & 0x1f;
            memcpy(&buffer[2], payload, psize);
        }

        void build(short streamID, unsigned int vector, bool exit){
            
            byte circuit = getNewPath((byte*) &vector);
            
            printBinary(circuit);

            this->buffer[0] = circuit;
            
            this->buffer[1] = 1<<6;
            
            if (exit)
                this->buffer[1] |= 1<<5;

            this->buffer[1] = streamID & 0x1f;

            getNextDirection();
        }

        void endBuild(short streamID, unsigned int vector, bool exit){
            memset(buffer, 0, PACKET);
            byte circuit = getNewPath((byte*) &vector);
            
            printBinary(circuit);

            this->buffer[0] = circuit;
            
            this->buffer[1] = 2<<6;
            
            if (exit)
                this->buffer[1] |= 1<<5;

            this->buffer[1] = streamID & 0x1f;

            getNextDirection();
        }

};

class Socks{
    private:
        byte buffer[300] = {0};
        byte ip[4] = {0};
        byte domain[255] = {0};
        byte domain_size = 0;
        int fd;
        unsigned short port = 0;

        void greetingsAnswer(int fd, int auth_method){
            byte answer[2];
            answer[0] = 0x05;
            if(auth_method){ // No method accepted
                answer[1] = 0xff;
            }else{ // Accept no_auth
                answer[1] = 0x00;
            }
            send(fd, answer, 2, 0);
        }

    public:
        Socks(int fd){
            this->fd = fd;
        }

        void handleGreetingPacket(){
            int nread = recv(this->fd, this->buffer, 64, 0);
            printf("Versao: %d\n", this->buffer[0]);
            if(nread < 0){
                // erro
            }else{
                if(this->buffer[0] == 0x05){
                    if(this->buffer[1] < 1){
                        // nao suporta nenhum tipo de autenticação
                        greetingsAnswer(fd, 1);
                    }else{
                        for(int i=2 ; i<nread; i++){
                            if(this->buffer[i] == 0x00){
                                greetingsAnswer(fd, 0);
                                memset(this->buffer, 0, 64);
                                // return 0;
                                break;
                            }
                        }
                    }
                }
                memset(this->buffer, 0, 64);
                greetingsAnswer(fd, 1);
            }
            // return 1;
        }
        
        int handleConnectionReq(){ // Ler pacote Client connection request 
	        int nread;
            if(nread = recv(this->fd, this->buffer, sizeof(this->buffer), 0) < 0){
                //erro
            }else{
                if(this->buffer[3] == 0x01){ // IPV4
                    memcpy(this->ip, &this->buffer[4], 4);
                    this->port = buffer[8] | buffer[9] << 8;
                    // printf(" %d.%d.%d.%d \n", this->ip[0], this->ip[1], this->ip[2], this->ip[3]);
                    memset(this->buffer, 0, nread);
                    return 0;
                }
                if(this->buffer[3] == 0x03){ // Domain
                    memcpy(this->domain, &this->buffer[5], this->buffer[4]);
                    domain_size = this->buffer[4];
                    memset(this->buffer, 0, nread);
                    // for(int i = 0; i < 255; i++){
                    //     printf("%c", this->domain);
                    // }
                    return 1;
                }
            }
            return -1;
        }

        byte* getIP(){
            return ip;
        }

        unsigned short getPort(){
            return port;
        }

        byte* getDomain(){
            return domain;
        }

        byte getDomainSize(){
            return domain_size;
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
    return -1;
}


void closeLocal(short streamID){
    close(localSocks[streamID].fd);
    localSocks[streamID].fd = -1;
    localSocks[streamID].events = 0;
    path[streamID] = 0;
}
void closeExit(pair<unsigned int,short> p){
    short streamID = streamIdentifier[p];
    close(exitSocks[streamID].fd);
    exitSocks[streamID].fd = -1;
    exitSocks[streamID].events = 0;
    streamIdentifier.erase(p);
    pathIdentifier.erase(streamID);
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

void proxyServerProcedure(int proxyClient){
    Socks clientsock(proxyClient);
    clientsock.handleGreetingPacket();
    int tipo = clientsock.handleConnectionReq(); // retorna se é ip ou domain
    if(tipo == 0){ // É ip
        byte *ip = clientsock.getIP();
        unsigned short port = clientsock.getPort();
        printf("%d.%d.%d.%d:%d \n", ip[0], ip[1], ip[2], ip[3], port);
    }
    if(tipo == 1){ // É domain
        byte *domain = clientsock.getDomain();
        printf("%s\n", domain);
        clientsock.getDomainSize();
    }

    // lock.aquire() ?
    // short streamid = findEmpty(localSocks, proxyClient);
}

//threadA
void proxyServerHandler(){
    proxyServer = tcpServerSocket(9999);
    while (1){
        struct sockaddr_in client = {0};
        socklen_t len = sizeof(struct sockaddr_in);
        int proxyClient = accept(proxyServer, (struct sockaddr*) &client, (socklen_t *)&len);
        if (proxyClient < 0){
            //algum tipo de erro
            close(proxyClient);
        } else {
            //sockv5 connection hnt n = andler
            ///....

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

int webConnect(byte type, byte* addr){
    uint16_t port = 0;
    int sock = -1;
    if (type){ // dominio
        int size = addr[0];

        char domain[size+1] = {0};
        memcpy(domain, &addr[1], size);

        port = (addr[size+1] << 8) | addr[size+2];
        char portchar[6];
        snprintf(portchar, 6, "%d", port);

        struct addrinfo *result, *rp;
        int r = getaddrinfo(domain, portchar, NULL, &result);

        if (r == 0){
            for (rp = result; rp != NULL; rp = rp->ai_next) {
                sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
                if (sock > 0) {
                    r = connect(sock, rp->ai_addr, rp->ai_addrlen);
                    if (r == 0) {
                        freeaddrinfo(result);
                        return sock;
                    } else {
                        close(sock);
                    }
                }
            }
        }
        freeaddrinfo(result);

    } else { // ipv4
        uint32_t ip = (addr[0]) | (addr[1] << 8) | (addr[2] << 16) | (addr[3] << 24);
        port = (addr[4] << 8) | (addr[5]);

        sock = tcpClientSocket(ip, port);
        return sock;
    }

    return -1;
}

void forwardingProcedure(short dir){

    short streamID = 0;
    if (dir < 4){//pacote da rede overlay

        Protocol packet(dir);
        packet.get();
        
        dir = packet.getNextDirection();//caso seja para encaminhar faz aqui a alteracao do salto
        streamID = packet.getStreamID();
        pair<unsigned int,short> p(packet.getVector(), streamID);

        switch(dir){
            case 0://é para encaminhar
                packet.forward();
                break; 
            case 1:{//new exitSocket
                pair<unsigned int,short> r(invert(p.first), streamID);

                int webSocket = webConnect();
                if (webSocket > 0){
                    short s = findEmpty(exitSocks, webSocket);
                    streamIdentifier[p] = s;
                    pathIdentifier[s] = r;
                }else{
                    packet.endBuild(streamID, r.first, false);
                    packet.forward();
                }
                break;
            }
            case 2://talk
                if (packet.isExitNode()){
                    send(exitSocks[streamIdentifier[p]].fd, NULL, PACKET, 0);
                }else{
                    if (path[streamID] == 0)
                        path[streamID] = invert(packet.getVector());
                    send(localSocks[streamID].fd, NULL, PACKET, 0);
                }
                break;
            case 3://end
                if (packet.isExitNode()){
                    closeExit(p);
                }else{
                    closeLocal(streamID);
                }
                break;
        }        
    } else {//manager

        ManagerHandler manager(1, 4420);
        manager.buildRegisterPacket();
        manager.sendManager();
        manager.get();
        if(manager.getPacketType() == 1){
            type = manager.getNodeTypeDir();
            manager.get();
            if(manager.getPacketType() == 2){
                uint32_t neighborIP = 0;
                int dir = manager.getNodeTypeDir();
                memcpy(&neighborIP, &manager.getPacket()[1], 4);
                int socketfd = tcpClientSocket(neighborIP, 4444);
                neighbors[dir].fd = socketfd;
                manager.buildNeighborPacket(dir);
                manager.sendNeighbor(dir);
            }
        }
        //novo vizinho( dir, ip )
        //estabelecer ligacao com o novo nó
        //avisar qual a dir do novo vizinho
    }
}

//threadC
void forwardingHandler(int manager, int type){
    
    short result = -1, streamID = -1, dir;
    byte* buffer = (byte*) malloc(PACKET);
    

    //poll atributes
    while (1){
        result = poll(neighbors, 5, -1);
        if (result < 0){
            //algum tipo de erro
        }

        dir = streamID = -1;
        while (dir++ < 5){
            if (neighbors[dir].revents & POLLIN){
                thread slave(forwardingProcedure, dir);
                slave.detach();
                neighbors[dir].revents = 0;
            }
            memset(buffer, 0, PACKET); 
        }
    }
}

void exitNodeProcedure(byte* buffer, short n, short streamID){
    Protocol packet(-1);
    packet.setPacket(buffer, n);

    pair<unsigned int, short> p = pathIdentifier[streamID];

    packet.build(p.second, p.first, false);
    packet.forward();
}

//threadD
void exitNodeHandler(){
    short streamID, result = -1, n;
    byte* buffer = (byte*)malloc(PACKET);
    
    while (1){
        result = poll(exitSocks, MAXFDS, -1);
        if (result < 0){
            //algum tipo de erro
        }
        streamID = -1;
        while (streamID++ < MAXFDS){
            if (exitSocks[streamID].revents & POLLIN){
                n = recv(exitSocks[streamID].fd, buffer, PACKET, 0);
                
                thread slave(exitNodeProcedure, buffer, n, streamID);
                slave.detach();
                
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
    ManagerHandler manager(1, 4420);
    
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

    thread startthread(proxyServerHandler);
    startthread.join();

    // 0 - 0 - 0 - x
    // 0 - 0 - 0 -
    // 0 - 0 - 0 -
    // |   |   |

    return 0;
}
