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



using namespace std;

typedef unsigned char byte;

#define MAXFDS  32
#define PACKET  1024
#define PAYLOAD 1022
#define MANAGERPACKET 128


struct pollfd nodesfds[MAXFDS] ={0};

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

class ThreadPool{
	typedef struct Node{
		int index;
		struct Node* next;
	}node;

	typedef void (*funcPtr) (int);

	private:
		node* head = NULL;
		funcPtr work = NULL;

		pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;								//
		pthread_cond_t cond = PTHREAD_COND_INITIALIZER;	
		pthread_t  *slave;	

		void push(int index){
			node* n = (node*)malloc(sizeof(node));
			n->index = index;
			n->next = NULL;
			if (head == NULL){
				head = n;
				return;
			}else{
				node* x = head;
				while(x->next != NULL){
					x = x->next;
				}
				x->next = n;
			}
		}

		int pop(){
			if (head == NULL){
				return -1;
			} else {
				node* n = head;
				head = head->next;
				int s = n->index;
				free(n);
				return s;
			}
		}

		void* pool(){
			int index;

			while (1){
				pthread_mutex_lock(&mutex);
				if ((index = pop()) == -1){
					pthread_cond_wait(&cond, &mutex);

					index = pop();
				}
			    pthread_mutex_unlock(&mutex);

			    if (index != -1){
			    	work(index);
			    }
			}
		}

		static void* jumpto(void* Object){
			return ((ThreadPool*)Object)->pool();
		}

	public:
		void init(int nthreads, void* t, void (*func)(int)){
			slave = (pthread_t*) malloc(nthreads*sizeof(pthread_t));
			work = func;
			for (int i = 0; i < nthreads; ++i){
				pthread_create(&slave[i], NULL, jumpto, (void*) t);
				pthread_detach(slave[i]);
			}
		}

		void sendWork(int index){
			pthread_mutex_lock(&mutex);
			push(index);
			pthread_cond_signal(&cond);
			pthread_mutex_unlock(&mutex);
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

        //stack<uint8_t> neighbors;
        
        stack<Node*> lost;

        byte buffer[MANAGERPACKET] = {0};

        pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
        
        int setNode(Node* node, unsigned int ip){
            int nc = 0, id = 0, i = 0;
            stack<pair<int, int>> neighbors;

            memset(buffer, 0, MANAGERPACKET);
            //resposta ao novo no
            buffer[0] = 2;
            
            buffer[1] = node->type;

            while (i < 4){
                if (id = getNeighbor(i, node->coord))
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
                Node* n = lost.top();
                n->update(id, socket);

                if (setNode(n, ip)){
                    lost.pop();
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
                lost.push(nodeList[id]);
                //new
                //Node *n = nodeList[id];
                //list.push(n, n->neighbors);
                
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

int tcpServerSocket(int port){
    int s = socket(AF_INET, SOCK_STREAM, 0);
    int one = 1;
    if (s < 0){
    	puts("Error: openning Socket descriptor     \n");
        return -1;
    }

    struct sockaddr_in addr = {AF_INET, htons(port)};
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind( s, (struct sockaddr *) &addr, sizeof(addr)) < 0){
    	close(s);
    	puts("Error: port binding                   \n");
        return -1;
    }
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) < 0) {
		close(s);
		puts("Error: SO_REUSEADDR                   \n");
		return -1;
	}
    if (setsockopt(s, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one)) < 0) {
		close(s);
		puts("Error: SO_REUSEPORT");
		return -1;
	}
    if (listen(s, MAXFDS) < 0){
        close(s);
        puts("Error: listen                         \n");
        return -1;
    }
    return s;  
}

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

int main(int argc,char** argv){    
    int client, manager, n = 1;

    struct sockaddr_in addr = {0};
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