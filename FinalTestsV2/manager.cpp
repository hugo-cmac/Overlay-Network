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

#include "criptography.h"
#include "tcp.h"
#include "threadpool.h"

using namespace std;

typedef unsigned char byte;

#define PACKET  1024
#define PAYLOAD 1022
#define MANAGERPACKET 128


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


template <typename T> class PriorityQueue {
    typedef struct List{
        T value;
        int n;
        struct List* next;
    }list;
    
    private:
       list* head = NULL;
    public:
        PriorityQueue(){};
        
        void push(T value, int n){
            list* temp = (list*) malloc(sizeof(list));
            temp->value = value;
            temp->n = n;
            temp->next = NULL;

            if (head == NULL){
                head = temp;
                head->next = NULL;

            }else{
                list* now = head;
                list* prev = NULL;

                while (now != NULL && now->n > n){//percorrer alertas
                    prev = now;
                    now = now->next;
                }
                if (now == NULL) {//final da lista
                    prev->next = temp;
                }else{
                    if(prev != NULL){//meio da lista
                        temp->next = now;
                        prev->next = temp;
                    }else{//inicio da lista
                        temp->next = head;
                        head = temp;
                    }
                }
            }               
        }

        T pop(){
            if (head != NULL){
                list* temp = head;
                head = head->next;
                T t = temp->value;
                free(temp);
                return t;
            }
            return NULL;
        }

        bool empty(){
            if (head == NULL)
                return true;
            return false;
        }

};

class Node{
    private:
        uint16_t id;
    public :
        byte type; //2-ligacoes/3-ligacoes/4-ligacoes
        coordinates coord = Coordinate(0,0);
        uint8_t neighbors;
        int sock = 0;

        Node(uint16_t id, uint16_t type, int sock, coordinates coord){
            this->id = id;
            this->type = type;
            this->coord = coord;
            this->sock = sock;
            this->neighbors = 0;
        }

        void update(uint16_t id, int sock){
            this->id = id;
            this->sock = sock;
        }

        void addNeighbor(){
            neighbors++;
            if(neighbors >= type){
                shutdown(sock,  SHUT_RDWR);
                close(sock);
            }
        }

        int sendMsg(byte* buffer){
            return send(sock, buffer, MANAGERPACKET, 0);
        }
};

class Matrix{

    private:
        uint16_t side ;
        uint16_t count ;
        coordinates coord = Coordinate(0,0);

        map<uint16_t, Node*> nodeList;
        map<coordinates, uint16_t> matrix;
        
        PriorityQueue <Node*> lost;

        byte buffer[MANAGERPACKET] = {0};

        pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
        
        int setNode(Node* node, unsigned int ip){
            int id = 0, i = 0;
            stack<pair<int, int>> neighbors;

            memset(buffer, 0, MANAGERPACKET);
            //resposta ao novo no
            buffer[0] = 2;
            
            buffer[1] = node->type;

            while (i < 4){
                if ((id = getNeighbor(i, node->coord)))
                    neighbors.push(make_pair(i, id));
                i++;
            }

            node->neighbors = 0;
            printf("Numero de vizinhos %ld\n", neighbors.size());
            buffer[2] = neighbors.size();

            if (node->sendMsg(buffer) > 0){
                // avisar vizinhos
                advertise(node, neighbors, ip);
                return 1;
            }
           return 0;
        }

        void advertise(Node* node, stack<pair<int,int>> neighbors, unsigned int ip){
            pair<int, int> temp;

            buffer[0] = 3;
            memcpy(&buffer[2], &ip, 4);

            while (!neighbors.empty()){
                temp = neighbors.top();
                
                buffer[1] = temp.first;
                if (nodeList[temp.second]->sendMsg(buffer) > 0){
                    nodeList[temp.second]->addNeighbor();
                    node->addNeighbor();
                }
                neighbors.pop();
            }
        }
        

        uint16_t getNeighbor(byte dir, coordinates coord){
            uint16_t id = 0;
            pthread_mutex_lock(&mutex);
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
            pthread_mutex_unlock(&mutex);
            return id;
        }

        void nextCoord(){
            if (count == side*side){
                side ++;
                coord.x = side - 1;
                coord.y = 0;
            } else {
                if (coord.x == side-1){
                    if (coord.y == side-2)
                        coord.x = 0;
                    coord.y++;
                } else {
                    coord.x++;
                }
            }
            //selectNeighbors();
        }

        byte getTypeOfNode(){
            if (coord.x == 0 && coord.y == 0)
                return 2;
            else if (coord.x == 0 || coord.y == 0)
                return 3;
            else
                return 4;
        }

        byte invert(byte dir){
            switch(dir){
                case UP:
                    return DOWN;
                case RIGHT:
                    return LEFT;
                case DOWN:
                    return UP;
                case LEFT:
                    return RIGHT;
            }
            return 0;
    }

    
    public:
        Matrix(){
            this->side = 1;
            this->count = 0; 
        }

        int newClient(uint16_t id, int socket, uint32_t ip){
            if (nodeList[id] != NULL){
                printf("Invalido\n");
                memset(buffer, 0, MANAGERPACKET);
                buffer[0] = 2;
                buffer[1] = 0;
                send(socket, buffer, MANAGERPACKET, 0);
                return 0;
            }
            
            if (!lost.empty()){
                printf("Take over %d\n", id);
                Node* n = lost.pop();
                n->update(id, socket);

                if (setNode(n, ip)){
                    nodeList[id] = n;
                    matrix[n->coord] = id;
                }
            } else {
                printf("New place %d\n", id);
                count++;
                Node* n = new Node(id, getTypeOfNode(), socket, coord);

                if (setNode(n, ip)){
                    nodeList[id] = n;
                    matrix[coord] = id;
                    nextCoord();
                }
            }
            return 1;
        }

        void lostNode(int id){
            pthread_mutex_lock(&mutex);
             printf("Reporting node: %d\n", id);
            if (nodeList.find(id) != nodeList.end()){
                printf("Node %d lost\n", id);
                
                //backup
                //lost.push(nodeList[id]);
                //new
                Node *n = nodeList[id];
                lost.push(n, n->neighbors);
                
                matrix[nodeList[id]->coord] = 0;
                nodeList.erase(id);
            }
            pthread_mutex_unlock(&mutex);
        }

        void checkNode(int id){
            if(nodeList[id]->neighbors < nodeList[id]->type){
                lostNode(id);
            }
        }

        void lostNeighbor(byte dir, uint16_t prevID, int sock){
            printf("Lost neighbor %d\n", dir);
            Node* n = nodeList[prevID];
            if (n == NULL){
                printf("NO inexistente id: %d\n", prevID);
                return;
            }
            
            if (sock != 0)
                n->sock = sock;
            
            uint16_t id = getNeighbor(dir, n->coord);
            n->neighbors--;

            lostNode(id);
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

Matrix matrix;

void clientHandler(int sock){
    byte buffer[MANAGERPACKET] = {0};
    int n = 1, id = 0;;
    
    while (n){
        n = recv(sock, buffer, MANAGERPACKET, 0);
        if (n > 0){
            if (buffer[0] == 4){
                id = (buffer[2] | (buffer[3] << 8));
                matrix.lostNeighbor(buffer[1], id, 0);
                continue;
            }
            //erro tipo invalido
        } else {
            if (id)
                 matrix.checkNode(id);
            close(sock);
            //desconectou-se
            n = 0;
        }
    }
}

int main(){    
    int client, manager, n = 1;

    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);

    ThreadPool slaves;
    slaves.init(20, (void*) &slaves, clientHandler);

    unsigned char buffer[MANAGERPACKET];
    while ((manager = tcpServerSocket(7777)) < 0)
        sleep(1);
    
    while(n){
        client = accept(manager, (struct sockaddr *)&addr, &len);
		if (client > 0){
            n = recv(client, buffer, MANAGERPACKET, 0);
            if (n > 0){
                if (buffer[0] == 1) { // register
                    if (matrix.newClient((uint16_t) (buffer[1] | (buffer[2] << 8)) , client, addr.sin_addr.s_addr))
                        slaves.sendWork(client);
                    else
                        close(client);
                    continue;
                } else if (buffer[0] == 4) { //patch up
                    matrix.lostNeighbor(buffer[1], (buffer[2] | (buffer[3] << 8)), client);
                    slaves.sendWork(client);
                    continue;
                }
                //unkown message
                continue;
            }
            n = 0;
            //erro do recv
        }
        //do accept
    }

    return 0;
}