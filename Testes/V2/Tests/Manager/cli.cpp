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
#include <stdio.h>



using namespace std; 

#define MANAGERPACKET 10
#define NEIGHBORS 4
#define byte unsigned char

short neighbors[NEIGHBORS];

enum DIRECTION {
    UP = 0,
    RIGHT,
    DOWN,
    LEFT
};
int overlayNodefd;

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
    addr.sin_addr.s_addr =ip;
    //inet_pton(AF_INET, ip, &addr.sin_addr); para char*
    if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) < 0){
        perror("Error connecting to server");
        return -1;
    }

    return s;
}

void logfile(char* message,int size){
    int filefd = open("log.txt",O_CREAT|O_APPEND|O_WRONLY,0666);
    write(filefd,message,size);
}

class ManagerHandler{
    private:
        uint32_t ip;
        int port;
        int manager;
        int dir;
        byte packet[MANAGERPACKET] = {0};

    public:
        ManagerHandler(uint32_t ip, int port){
            this->ip = ip;
            this->port= port;
        }

        int connectToManager(){
            if((manager = tcpClientSocket(ip, port))){
                return 1;
            }
            return 0;
        }
        byte* getPacket(){
            return packet;
        }
        
        int getNodeType(){
            return((packet[0] & 0x38) >> 3);
        }

        int getPacketType(){
            return ((packet[0] & 0xc0)>>6);
        }
        int getConnectingNeighbors(){
            
            return ((packet[0] & 0x03));
        }
        int getNeighborDir(){
            return((packet[0] & 0x38)>>3);
        }
        void buildRegisterPacket(uint8_t id){
            packet[0] = 0;
            packet[1] = id; //certificado      
        }

        void buildNeighborPacket(int dir){
            packet[0] = 3;
            packet[1] = invertDir(dir);
        }

        int sendManager(){
            int n=send(manager, packet, MANAGERPACKET, 0);
            memset(packet, 0, MANAGERPACKET);
            return n;
        }

        int get(){
            memset(packet, 0, MANAGERPACKET);
            int n = recv(manager, packet, MANAGERPACKET, 0);
            return n;
        }

        uint32_t getNeighborIp(){
            return (packet[1] | (packet[2] << 8) | (packet[3] << 16) | (packet[4] << 24));
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
            return -1;
        }
};



ManagerHandler* manager;

void neighborHandler(int fd){
    unsigned char buffer[65536]={0};
    while(recv(fd,buffer,sizeof(buffer),0)){
        cout << buffer<< "\n";
    }
}

void awaitConnectionsFromNeighbors(int nr){
    struct sockaddr_in neighboraddr;
    socklen_t addrlen;
    byte buffer;
    int neighborfd;
    while(nr){
        if((neighborfd = accept(overlayNodefd,(sockaddr *)&neighboraddr,&addrlen))){
            char aux[20] = "New neighbor\n";
            logfile(aux,sizeof(aux));
            if(recv(neighborfd,&buffer,1,0)){
                switch(buffer){
                    case 0 : 
                        cout << "Em cima\n";
                        break;
                    case 1 : 
                        cout << "Direita\n";
                        break;
                    case 2 : 
                        cout << "Baixo\n";
                        break;
                    case 3: 
                        cout << "Esquerda\n";
                        break;
                }
                neighbors[buffer] = neighborfd;
                thread t(neighborHandler,neighborfd);
                t.detach();
            }
        }
    }
       
}

void connectToNeighbors(){
    int neighborfd;
    //cout <<"Waiting orders\n";
    while(manager->get()){
        if(manager->getPacketType()==2){
            if((neighborfd = tcpClientSocket(manager->getNeighborIp(),5555))){
                //cout << "Connecting to neighbor";
                //getchar();
                uint8_t dir = manager->getNeighborDir();

                send(neighborfd,&dir,1,0);
                send(neighborfd,"OLA",3,0);
                
                neighbors[manager->invertDir(manager->getNeighborDir())]=neighborfd;
                thread t(neighborHandler,neighborfd);
                t.detach();
            }
        }
    }
}


//   2bits(0)     6bits |      8bits
// <packet type>   <>   |   <client ID>  -> registo do cliente
//manager <- client

//   2bits(1)         3bits            3bits
// <packet type>  <type of node>  <awaited connect>   -> resposta do manager
//manager -> client

//   2bits(2)         3bits        3bits |  32bits
// <packet type>   <direction>      <>   |   <IP>  -> connect to new node
//manager -> client

//   2bits(3)         3bits      |      8bits
// <packet type>   <direction>   |   <client ID>  -> Patch up
//manager <- client

//   2bits(2)          3bits       3bits |
// <packet type>    <direction>     <>   |
//client -> client

int main(int argc,char** argv){
    
    int numberOfNeighbors;
    uint32_t ip;
    uint8_t id = (uint8_t) atoi(argv[1]);
    
    inet_pton(AF_INET, "10.0.0.10",&ip);
    manager = new ManagerHandler(ip, 7777);

    if(manager->connectToManager()){
    
        manager->buildRegisterPacket(id);
        manager->sendManager();

        overlayNodefd = tcpServerSocket(5555);
    
        if(manager->get()){
            if(manager->getPacketType() == 1){
                numberOfNeighbors=manager->getNodeType();
                //printf("NUmber: %d\n",numberOfNeighbors);
                int neighborsFromManager = numberOfNeighbors - manager->getConnectingNeighbors();
                //cout << "Neighbors from manager " << neighborsFromManager<<"\n";
                //cout << "ConnectingNeighbors()" << manager->getConnectingNeighbors()<<"\n";
                if(manager->getConnectingNeighbors()){
                    //cout << "Await from neighbors\n";
                    thread t(awaitConnectionsFromNeighbors,manager->getConnectingNeighbors());
                    t.detach();
                }
                if(neighborsFromManager){
                    connectToNeighbors();
                }
            }else{
                cout << manager->getPacketType() << "\n";
            }
        }else{
            cout << "ERROR\n";
        }
    }

   

}