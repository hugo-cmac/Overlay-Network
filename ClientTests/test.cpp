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
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include <bits/stdc++.h>
#include <pthread.h>

using namespace std;

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

typedef unsigned char byte;

#define MAXFDS  32
#define PACKET  1024
#define PAYLOAD 1022

enum DIRECTION {
    UP = 0,
    RIGHT = 1,
    DOWN = 2,
    LEFT = 3
};

enum CONTENTTYPE{
    NEW = 0,
    TALK = 1,
    END = 2 
};



//////////////////////////////////////////////////////////////////////////////////////////
// Exit Sockets                                                                     	//
int  exitSocks [MAXFDS] = {0};                                            				//
// < < vector, streamID >, streamID>                    	   ////// //  // // /////// //
map<pair<unsigned int, short>, short> streamIdentifier; 	   //      ////  //   //    //
//                                                      	   /////    //   //   //    //
// < streamID, < vector, streamID >                     	   //      ////  //   //    //
map<short, pair<unsigned int, short>> pathIdentifier;   	   ////// //  // //   //    //
// Thread control																		//
ThreadPool exitstack;																	//
//////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////
//                                                                                 		//
// Path to exit node                            	 //     ////// //////  ////  //     //
unsigned int path[MAXFDS] = {0};                	 //     //  // //     //  // //     //
//                                              	 //     //  // //     ////// //     //
// Local Sockets                                	 //     //  // //     //  // //     //
int localSocks [MAXFDS] = {0};        				 ////// ////// ////// //  // ////// //
//Thread control																		//
ThreadPool local;																    	//
//////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////
//																						//
short availableNeighbor[4] = {0};	//  // ////// // ////// //  // /////  ////// /////	//
short type;							/// // //     // //  	//  // //  // //  // //  //	//
int neighbors [5] = {0}; 			////// /////  // // ///	////// /////  //  // /////	//
//									// /// //     // //  //	//  // //  // //  // // //	//
//									//  // ////// // //////	//  // /////  ////// //  //	//
//Thread control																		//
ThreadPool neighbor;																	//
//////////////////////////////////////////////////////////////////////////////////////////




unsigned int timeR = time(NULL);
int random(int n){
    timeR++;
    srand( timeR);
    return rand() % n;
}

void printBinary(byte b){
    short i = 8;
    while(i--){
        if (((b>>i) & 0x01) == 1){
			putchar('1');putchar(' ');
        }else{
            putchar('0');putchar(' ');
		}
    } 
    putchar('\n');
}

void printBytes(byte* buffer, int size){
	for (int i = 0; i < size; ++i)
		printf("[%02X]",buffer[i]);
	printf("\n");
}

int tcpServerSocket(int port){
    int s = socket(AF_INET, SOCK_STREAM, 0);
    int one = 1;
    if (s < 0){
    	puts("\x1b[34;0fError: openning Socket descriptor                     \n");
        return -1;
    }
    struct sockaddr_in addr = {AF_INET, htons(port)};
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind( s, (struct sockaddr *) &addr, sizeof(addr)) < 0){
    	close(s);
    	puts("\x1b[34;0fError: port binding                      \n");
        return -1;
    }
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) < 0) {
		close(s);
		puts("\x1b[34;0fError: SO_REUSEADDR                    \n");
		return -1;
	}
    if (listen(s, MAXFDS) < 0){
        close(s);
        puts("\x1b[34;0fError: listen                         \n");
        return -1;
    }
    return s;  
}

int tcpClientSocket(uint32_t ip, uint16_t port){
    int s = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {AF_INET, htons(port)};
    addr.sin_addr.s_addr = ip;

    if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) < 0){
    	close(s);
        puts("\x1b[34;0fError: connecting to server                        \n");
        return -1;
    }
    return s;
}

int webConnect(byte type, byte* addr){
	uint16_t port = 0;
	int sock = -1;
	if (type){ // dominio
		int size = addr[0];

		char domain[size+1] = {0};
		memcpy(domain, &addr[1], size);

		char printbuffer[256];
		sprintf(printbuffer,"\x1b[35;0fDomain: %s", domain);
		puts(printbuffer);

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

class Protocol{
    private:
        byte *buffer = NULL;
        int n = 0;
		byte listenDirection;
        byte direction = -1;

        byte getRandomDirection(byte hop){
			puts("Random");
            int i = random(4);
            while(!availableNeighbor[i] || listenDirection == i){
                i = random(4);
            }
            byte dir = i;
            buffer[0] |= (dir & 0x03)<<(hop<<1);
        
            return dir;
        }

		byte getNewPath(byte vector[4]){
			puts("Get nw path");
            int dir[3]={0};
            int f = 0, s = 0, t = 0;
			
            while(f < 4){
                if (vector[f]){
                    vector[f] -= 1;
                    dir[s++] = f;
					printf("%d ", dir[s-1]);
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

		byte recalc(byte hop){
			puts("------->recalc");
			byte f, s;
			if(hop == 2){//11 00 00 11
				f = buffer[0]>>4 & 0x03;
				s = buffer[0]>>2 & 0x03;
				buffer[0] &= 0xc3;
				buffer[0] |= f<<2 | s<<4;
				return s;
			}
			puts("Erro no recalc");
			return 5;//erro
		}

    public:

        Protocol(short dir){
        	if (dir != -1){
        		buffer = (byte*) malloc(PACKET);
				this->listenDirection = dir;
			}
            this->direction = dir; 
        }

        //NEIGHBORS INPUT/OUTPUT
        int get(){
        	memset(buffer, 0, PACKET);
            n = recv(neighbors[listenDirection], buffer, PACKET, 0);
            if (n < 0)
                return -1;
			puts("Recebeu");
            return n;
        } 
        int forward(){
            n = send(neighbors[direction], buffer, PACKET, 0);
            if (n < 0)
                return -1;
			puts("Enviou");
            return n;
        }

        //PACKET PARAMETER RELATED
        short getNextDirection(){
			puts("Get next Direction");
            //   6bit     2bits | 2bits     1bits(0/1)      5bits    |   Resto
            // <circuito> <hop> | <new>    <ip/dominio>   <streamID> | <Payload>

            //                    2bits      1bit(0/1)      5bits    |   Resto
            // <circuito> <hop> | <talk>   <local/exit>   <streamID> | <Payload>
            // <circuito> <hop> | <end>    <local/exit>   <streamID> | errr - qual foi

            // <circuito> <hop> | <resp>      <local>     <streamID> | <Payload>

            byte type = (buffer[1]>>6) & 0x03;//saber se é new ou nao
            byte hops = (buffer[0]) & 0x03;//numero de hops
			printf("type: %d\n", type);
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
				printf("hops: %d\n", hops);
                if (type == NEW){//saber se é new ou nao
                    direction = getRandomDirection(hops);
                }else{
                    direction = (buffer[0]>>(hops<<1)) & 0x03;
					if(!availableNeighbor[direction]){
						direction = recalc(hops);
					}
                }
                hops -= 1;//decrementar saltos
                buffer[0] = (buffer[0] & 0xfc) | (hops & 0x03);
            }
			printf("dir: %d\n", direction);
			printBinary(buffer[0]);
            return 0;
        }
        unsigned int getVector(){
            unsigned int vector = 0;    
            int temp=0;
                    
            temp = (buffer[0]>>2) & 0x03;
            vector += 1<<(temp<<3);

            temp = (buffer[0]>>4) & 0x03;
            vector += 1<<(temp<<3);
                    
            temp = (buffer[0]>>6) & 0x03;
            vector += 1<<(temp<<3);

            return vector;
        }
        short getStreamID(){	return buffer[1] & 0x1f;
        }
        bool isExitNode(){
            if ((buffer[1]>>5) & 0x01) 
                return true;
            return false;
        }

        byte* getPayload(){		return &buffer[2];
        }

        byte getAddrType(){		return (buffer[1] & 0x20)>>5;
        }

        byte* getAddr(){ 		return &buffer[2];
        }

        //PACKET CREATION
        void freshBuild(bool domain, short streamID, byte* payload, int psize){
            direction = getRandomDirection(3);
            buffer[0] |= 2 & 0x03;

            buffer[1] = 0<<6;
            if (domain)
                buffer[1] |= 1<<5;

            buffer[1] = streamID & 0x1f;
            memcpy(&buffer[2], payload, psize);
        }

        void build(short streamID, unsigned int vector, bool exit, byte* payload){
            buffer = payload;

            byte circuit = getNewPath((byte*) &vector);
            buffer[0] = circuit;
            buffer[1] = 1<<6;

            if (exit)
                buffer[1] |= 1<<5;
            buffer[1] = streamID & 0x1f;
            getNextDirection();
        }

        void endBuild(short streamID, unsigned int vector, bool exit){
			puts("build end");
            memset(buffer, 0, PACKET);
			
            byte circuit = getNewPath((byte*) &vector);
            
            buffer[0] = circuit;
            
            buffer[1] = 2<<6;
            
            if (exit)
                buffer[1] |= 1<<5;

            buffer[1] |= streamID & 0x1f;

            getNextDirection();
        }

        void buildTest(byte* payload){
			puts("Build test");
        	buffer = payload;
        	direction = getRandomDirection(3);//dir
			
        	buffer[0] |= 2 & 0x03;//hops

			printBinary(buffer[0]);
        }

        void setPacket(byte* buffer, short n){
            memcpy(&buffer[2], buffer, n);
        }

};

unsigned int invert(unsigned int vector){
    unsigned int inverse = 0;
    inverse |= (vector>>16 & 0x03);
    inverse |= (vector>>24 & 0x03)<<8;
    inverse |= (vector & 0x03)<<16;
    inverse |= (vector>>8  & 0x03)<<24;

    return inverse;
}

byte invertDir(byte dir){
	switch(dir){
		case 0x00:
			return 0x02;
		case 0x01:
			return 0x03;
		case 0x02:
			return 0x00;
		case 0x03:
			return 0x01;
	}
	return 0;
}

short findEmpty(int* list, int fd){
    short i = -1;
    while (i++ < MAXFDS){
        if (list[i] == -1){   
            list[i] = fd;
            return i;
        }
    }
    return -1;
}

void closeExit(pair<unsigned int,short> p){
    short streamID = streamIdentifier[p];
    shutdown(exitSocks[streamID], SHUT_RDWR);
	close(exitSocks[streamID]);
    exitSocks[streamID] = -1;
    streamIdentifier.erase(p);
    pathIdentifier.erase(streamID);
}

void closeLocal(short streamID){
    shutdown(localSocks[streamID], SHUT_RDWR);
	close(localSocks[streamID]);
    localSocks[streamID] = -1;
    path[streamID] = 0;
}

//threadD
void exitNodeHandler(int streamID){
    int n = 1;

    byte* buffer = (byte*)malloc(PACKET);
    Protocol packet(-1);

    while (n){
    	memset(buffer, 0, PACKET);
		n = recv(exitSocks[streamID], &buffer[2], PAYLOAD, 0);
		pair<unsigned int, short> p = pathIdentifier[streamID];

		if (n > 0) {
			packet.build(p.second, p.first, false, buffer);
			packet.forward();
		} else {
			packet.endBuild(streamIdentifier[p], p.first, false);
			packet.forward();
			closeExit(p);
			return;
		}
    }
}

//threadC
void* forwardingHandler(void* direc){
    int n = 1, streamID, result;
    int direction = *(int*) direc;
    Protocol packet(direction);

    //poll atributes
    while (n){
    	if (packet.get() > 0){
			streamID = 0;
		    result = packet.getNextDirection();//caso seja para encaminhar faz aqui a alteracao do salto
		    streamID = packet.getStreamID();
		    pair<unsigned int,short> p(packet.getVector(), streamID);

	        switch(result){
		       	case 0://é para encaminhar    
		            packet.forward();
					puts("Forward");
		            break; 
		        case 1:{//new exitSocket
					puts((char*) packet.getPayload());
					
					packet.endBuild(0, invert(packet.getVector()), true);
					packet.forward();
		            // pair<unsigned int,short> r(invert(p.first), streamID);
		            // int webSocket = webConnect(packet.getAddrType(), packet.getAddr());
		            // if (webSocket > 0){
		            //     short s = findEmpty(exitSocks, webSocket);
		            //     streamIdentifier[p] = s;
		            //     pathIdentifier[s] = r;
		            //     //send work to exitsocket

		            // }else{
		            //     packet.endBuild(streamID, r.first, false);
		            //     packet.forward();
		            // }
		            break;
		        }
		        case 2://talk
		            if (packet.isExitNode()){//exit sock
		                //send(exitSocks[streamIdentifier[p]], packet.getPayload(), PACKET, 0);
		                cout << "Exit: "<< (char*)packet.getPayload()<<"\n";
		            }else{//localsock
		            	cout << "Local: "<< (char*)packet.getPayload()<<"\n";
		                // if (path[streamID] == 0)
		                //     path[streamID] = invert(packet.getVector());
		                // send(localSocks[streamID], packet.getPayload(), PACKET, 0);
		            }
		            break;
		        case 3://end
					puts("Recebeu resposta");
		            // if (packet.isExitNode()){
		            //     closeExit(p);
		            // }else{
		            //     closeLocal(streamID);
		            // }
		            break;
		    }       
		} else {
			//vizinho com o crlh
			return NULL;
		}
    }
    return NULL;
}

void threadA(){

}



int main(int argc, char const *argv[]){
	//ligacao ao manager
	//verificacao do manager
	//manager diz o tipo
	//recebe ligacoes
	
	struct sockaddr_in client = {0};
    socklen_t len = sizeof(struct sockaddr_in);
	int clients;

	int aux;
	pthread_t slave[4];
	type = atoi(argv[1]);
	byte buffer[PACKET];
	


	if (type == 2){
		for (int i = 0; i < 2; ++i){
			cout<<"Type: "<< type<<"\nChoose neighbor:\n";
			cout<<"  0\n3   1\n  2\n";

			scanf("%d", &aux);
			buffer[0] = aux;
			inet_pton(AF_INET, argv[i+2], &client.sin_addr);
		
			neighbors[aux] = tcpClientSocket(client.sin_addr.s_addr, 9999);
			availableNeighbor[aux] = 1;

			send(neighbors[aux], buffer, PACKET, 0);
			pthread_create(&slave[i], NULL, forwardingHandler, (void*) (&aux));
			pthread_detach(slave[i]);
		}

		
	} else if(type == 3){
		
		int server = tcpServerSocket(9999);
		clients = accept(server, (struct sockaddr*) &client, (socklen_t *)&len);
		recv(clients, buffer, PACKET, 0);
		aux = invertDir(buffer[0]);
		cout<<"New connection on "<<aux<<"\n";

		neighbors[aux] = clients;
		availableNeighbor[aux] = 1;

		pthread_create(&slave[0], NULL, forwardingHandler, (void*) (&aux));
		pthread_detach(slave[0]);

		cout<<"Type: "<< type<<"\nChoose neighbor:\n";
		cout<<"  0\n3   1\n  2\n";

		scanf("%d", &aux);
		buffer[0] = aux;
		inet_pton(AF_INET, argv[2], &client.sin_addr);
		
		neighbors[aux] = tcpClientSocket(client.sin_addr.s_addr, 9999);
		availableNeighbor[aux] = 1;

		send(neighbors[aux], buffer, PACKET, 0);
		pthread_create(&slave[1], NULL, forwardingHandler, (void*) (&aux));
		pthread_detach(slave[1]);

		close(server);
		
	} else if (type == 4) {
		int server = tcpServerSocket(9999);
		for (int i = 0; i < 2; ++i){
			clients = accept(server, (struct sockaddr*) &client, (socklen_t *)&len);
			recv(clients, buffer, PACKET, 0);

			aux = invertDir(buffer[0]);
			cout<<"New connection on "<<aux<<"\n";
			
			neighbors[aux] = clients;
			availableNeighbor[aux] = 1;

			pthread_create(&slave[i], NULL, forwardingHandler, (void*) (&aux));
			pthread_detach(slave[i]);
		}
		puts("Configurado");
		close(server);
	}
	
	Protocol p(-1);	
	while(1){
		memset(buffer,0,PACKET);
		read(0, &buffer[2], PAYLOAD);
		p.buildTest(buffer);
		p.forward();
	}

	return 0;
}