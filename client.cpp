//c++ libs
#include <iostream>
#include <map>
#include <stack>
#include <string>
#include <iterator>
#include <cstring>

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

struct pollfd localSocks [MAXFDS] = {0};
struct pollfd  exitSocks [MAXFDS] = {0};

//objeto para x509
//objeto para echd
//objeto para pipes         1B          1B            2b            1b                  2b                     2b                       
//objeto para ler pacotes  <tp> <circuito 1byte > <saltos 4> <flag exit/local> <flag new/talk/end> <flag ip/domain/payload> <payload>
//procedimento de sockv5/v4
//procedimento de abertura de exitSocks(map para streams)


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
    addr.sin_addr.s_addr = ip
    //inet_pton(AF_INET, ip, &addr.sin_addr); para char*

    if (connect(s, (struct sockaddr *)&addr,sizeof(addr)) < 0){
        perror("Error connecting to server");
        return -1;
    }

    return s;
}

//threadA
void proxyServerHandler(int pipe){
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

            //se tudo der correto
            short i = -1;
            while (i++ < MAXFDS){
                if (localSocks[i].fd == -1){
                    write(pipe, &i, 2);
                    //write(pipe, buffer, PACKET); new circuit
                    localSocks[i].fd = proxyClient;
                    localSocks[i].events = POLLIN;
                    break;
                }
            }
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

//threadC
void forwardingHandler(int manager, int pipe, int type){
    map<short, byte> stream;

    struct pollfd neighbor[6];
    short result = -1, streamID = -1, dir;

    byte* buffer = (byte*)malloc(PACKET);



    //poll atributes
    while (1){

        result = poll(neighbor, 6, -1);

        if (result < 0){
            //algum tipo de erro
        }

        dir = streamID = -1;

        while (dir++ < 6){

            if (neighbor[dir].revents & POLLIN){

                if (dir < 5){//protocolo

                    recv(neighbor[dir].fd, buffer, PACKET, 0);

                    //desecriptar(dir, buffer)

                    if (dir < 4){//pacote da rede overlay
                        /*
                        dir = getDirection(buffer)//caso seja para encaminhar faz aqui a alteracao do salto

                        if (dir < 0) { //é para aqui

                            streamID = isExitNode(buffer)// devolve streamID
                            //adicionar aqui flag de fecho de descritor

                            if (streamID < 0) { // exit sock

                                send(exitSocks[-streamID].fd, buffer)

                            } else { // local sock

                                send(localSocks[streamID].fd, buffer)

                            }

                        } else {//é para encaminhar

                            send(neighbor[dir].fd, buffer)

                        }
                        */
                    } else {//manager
                        //novo vizinho( dir, ip )
                        //estabelecer ligacao com o novo nó
                        //avisar qual a dir do novo vizinho
                    }
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
