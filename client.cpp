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

byte availableNeighbor[4];//var[0]=01/var[1]=10
short type;//=2 vertice

struct pollfd localSocks [MAXFDS] = {0};
struct pollfd  exitSocks [MAXFDS] = {0};
struct pollfd  neighbors [4]      = {0};

map<pair<unsigned int, short>, byte> streamIdentifier;
map<byte, pair<unsigned int, short>> pathIdentifier;

byte path[MAXFDS];

//objeto para x509
//objeto para echd
//                          1B          1B            2b            1b                  2b                     2b                       
//objeto para ler pacotes  <tp> <circuito 1byte > <saltos 4> <flag exit/local> <flag new/talk/end> <flag ip/domain/payload> <payload>
//                                  
//procedimento de sockv5/v4
//procedimento de abertura de exitSocks(map para streams)


class Protocol{
    private:
        byte* buffer = NULL;
        byte mask = 0x03;//0xfc

        //byte circuit = 0;

        short nb = 0;
        byte direction = -1;

        byte getRandomDirection(byte hop){
            srand( time( NULL));
            int i = rand() % type;

            byte dir = availableNeighbor[i];
            this->buffer[0] |= (dir & this->mask)<<(hop<<1);
        
            return dir;
        }

        byte getNewPath(byte circuit){
            int dir[3] = {
                (circuit>>6) & mask,
                (circuit>>4) & mask,
                (circuit>>2) & mask
            };

            srand( time( NULL));
            int i = rand() % 6;
            byte newCircuit = 0;

            switch(i){
                case 0:
                    newCircut = dir[0]<<6 | dir[1]<<4 | dir[2]<<2 | 3;
                    break;
                case 1:
                    newCircut = dir[0]<<6 | dir[2]<<4 | dir[1]<<2 | 3;
                    break;
                case 2:
                    newCircut = dir[1]<<6 | dir[0]<<4 | dir[2]<<2 | 3;
                    break;
                case 3:
                    newCircut = dir[1]<<6 | dir[2]<<4 | dir[0]<<2 | 3;
                    break;
                case 4:
                    newCircut = dir[2]<<6 | dir[0]<<4 | dir[1]<<2 | 3;
                    break;
                case 5:
                    newCircut = dir[2]<<6 | dir[1]<<4 | dir[0]<<2 | 3;
                    break;
            }
            return newCircuit;
        }

    public:
        Protocol(short dir){
            this->direction = dir;
            
            this->buffer = (byte*) malloc(PACKET);
            
        }

        //~Protocol();


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
                if (type == NEW){//saber se é new ou nao
                    return 1;
                }else{
                    //getstream
                    return 2;
                }
            }else{
                if (type == NEW){//saber se é new ou nao
                    this->direction = getRandomDirection(hops);
                }else{
                    this->direction = (this->buffer[0]>>(hops<<1)) & mask;
                }
                hops -= 1;//decrementar saltos
                this->buffer[0] = (this->buffer[0] & 0xfc) | (hops & mask);
                return 0;
            }
        }
        byte getCircuit(){
            return this->buffer[0] & 0xfc;
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

        //PACKET CREATION
        void build(short streamID, byte circuit, bool exit){
            
            circuit = getNewPath(circuit);
            this->buffer[0] = circuit;
            
            this->buffer[1] = 1<<6;
            
            if (exit)
                this->buffer[1] |= 1<<4;

            this->buffer[2] = streamID & 0x3f;

            getNextDirection();
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

            short i = -1;
            while (i++ < MAXFDS){
                if (localSocks[i].fd == -1){
                    
                    localSocks[i].fd = proxyClient;
                    localSocks[i].events = POLLIN;
                    break;
                }
            }
            //sq fazer algo
        }
    }
}

void proxyLocalProcedure(byte* buffer, short n, short streamID){
    Protocol packet(-1);
    packet.setPacket(buffer, n);

    packet.build(streamID, path[streamID], false);
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

void forwardingProcedure(short dir){
    
    short streamID = 0;

    Protocol packet(dir);
    packet.get();

    if (dir < 4){//pacote da rede overlay
        
        dir = packet.getNextDirection();//caso seja para encaminhar faz aqui a alteracao do salto

        if (dir) { //é para aqui

            streamID = packet.getStreamID();
            if (dir == 1){
                //abrir exitsocket

            }else{
                if (packet.isExitNode()){
                    //procura no mapa por circuit,streamID == exitstreamID
                    send(exitSocks[1].fd, NULL, PACKET, 0);
                }else{
                    if (path[streamID] == 2)
                        path[streamID] = packet.getCircuit();
                    
                    send(localSocks[streamID].fd, NULL, PACKET, 0);
                }
            }
        } else {//é para encaminhar
            packet.forward();
        }
        
    } else {//manager
        //novo vizinho( dir, ip )
        //estabelecer ligacao com o novo nó
        //avisar qual a dir do novo vizinho
    }

}

//threadC
void forwardingHandler(int manager, int pipe, int type){
    

    short result = -1, streamID = -1, dir;

    byte* buffer = (byte*)malloc(PACKET);
    
   

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
    memset(path, 2, MAXFDS);
       

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
