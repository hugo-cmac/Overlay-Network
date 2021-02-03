#include <iostream>
#include <map>
#include <stack>
#include <string>
#include <iterator>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <string.h>
#include <signal.h>
#include <poll.h>
#define MAXFDS 32
using namespace std;

int manager_fd;

struct pollfd nodesfds[MAXFDS] = {0};

enum DIRECCION {
    UP = 0,
    RIGHT,
    DOWN,
    LEFT
};

typedef struct Coordinate{
    uint16_t x;
    uint16_t y;

    Coordinate(uint16_t x, uint16_t y): x(x), y(y){}
    
    bool operator<(const Coordinate &ob) const{
		return x < ob.x || (x == ob.x && y < ob.y);
	}
    struct Coordinate left(){
        return Coordinate(x-1, y);
    }
    struct Coordinate right(){
        return Coordinate(x+1, y);
    }
    struct Coordinate up(){
        return Coordinate(x, y-1);
    }
    struct Coordinate down(){
        return Coordinate(x, y+1);
    }
    
}coordinates;


class Node{
    private:
        uint16_t certificate;
        uint16_t id;
        uint8_t type; //2-ligacoes/3-ligacoes/4-ligacoes
        int socket_fd = 0;
        uint32_t ip;
        coordinates coord = Coordinate(0,0);
    public :
        Node(uint16_t certificate, uint16_t id, uint16_t type, int fd, coordinates coord){
            this->certificate = certificate;
            this->id = id;
            this->type = type;
            this->coord = coord;
            this->socket_fd = fd;
        }

        coordinates getCoords(){
            return coord;
        }

        uint8_t getType(){
            return this->type;
        }

        int getSocketFD(){
            return this->socket_fd;
        }

        int sendMsg(unsigned char* buffer,int size){
            return send(this->socket_fd,buffer,size,0);
        }

};

class Matrix{

    private:
        uint16_t side ;
        uint16_t count ;

        coordinates coord = Coordinate(0,0);

        map<uint16_t, Node*> NodeList;
        map<coordinates, uint16_t> matrix;

        stack<uint8_t> neighbors;
        stack<coordinates> lost;

    public:
        Matrix(){
            this->side = 1;
            this->count = 0; 
        }

        void newClient(uint16_t id, int socket, uint32_t ip){
            
            count++;
            if(!lost.empty()){
                coordinates aux = lost.top();
                

            }else{
                Node *n = new Node(0, id, getTypeOfNode(), socket, coord);
                NodeList[id] = n;
                matrix[coord] = id;
                advertiseNeighbors(coord,ip);
                nextCoord();
                selectNeighbors();
            }
            
        }

        int advertiseNeighbors(coordinates coord,uint32_t ip){
            uint8_t dir;
            uint16_t id=0;
            unsigned char msg[5] = {0};

            msg[0] = 2 << 3;
            memcpy(&msg[1],&ip,4);
            while (!neighbors.empty()){
                dir = neighbors.top();
                id = getNeighbor(dir,coord);
                neighbors.pop();
                msg[0] |= dir;
                Node* neighbor = NodeList[id];
                neighbor->sendMsg(msg,5);
            }
                
        }

        uint16_t getNeighbor(uint8_t dir,coordinates coord){
            uint16_t id = 0;
            switch (dir) {
                case UP:
                    id = matrix[coord.up()];
                    break;
                case RIGHT:
                    id = matrix[coord.right()];
                    break;
                case DOWN:
                    id = matrix[coord.down()];
                    break;
                case LEFT:
                    id = matrix[coord.left()];
                    break;
            }
            return id;
        }

        void selectNeighbors(){
            if (coord.x) 
                neighbors.push(LEFT);
            if (coord.y) 
                neighbors.push(UP);
        }

        void nextCoord(){
            if (count == side*side){
                side ++;
                coord.x = side - 1;
                coord.y = 0;
            } else {
                if (coord.x == side-1){
                    if (coord.y == side-2){
                        coord.x = 0;
                    }
                    coord.y++;
                } else {
                    coord.x++;
                }
            }
        }

        int getTypeOfNode(){
            if (coord.x == 0 && coord.y == 0){
                return 2;
            } else if (coord.x == 0 || coord.y == 0) {
                return 3;
            } else {
                return 4;
            }
        }

        void printMatrix(){
            coordinates c = Coordinate(0,0);

            for (size_t y = 0; y < side; y++){
                for (size_t x = 0; x < side; x++){
                    c.x = x;
                    c.y = y;
                    cout << "Node: " + to_string(matrix[c]) + "   ";
                }
                cout << "\n";
            }
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
void closefd(int sign){
    close(manager_fd);
}

void maintenanceModule(){

}

int main(int argc,char *argv){
    signal(SIGINT,closefd);
    Matrix* matrix = new Matrix();

    int node_fd,n;
    struct sockaddr_in manager, node;
    socklen_t nodelen;
    unsigned char buffer[1500];
    if((manager_fd = tcpServerSocket(7777))<0){
        exit(1);
    }
    nodelen = sizeof(node);
    node = {0};
    while(1){
        n=0;
        if ((node_fd = accept(manager_fd, (struct sockaddr *)&node,&nodelen)) < 0){
			perror("accept()");
			exit(1);
		}
        if((n=recv(node_fd,buffer,sizeof(buffer),0))<=0){
            
        }else{
            if(buffer[0]==0){
                matrix->newClient((uint16_t) buffer[1],node_fd,node.sin_addr.s_addr);
            }
        }

    }

    return 0;
}