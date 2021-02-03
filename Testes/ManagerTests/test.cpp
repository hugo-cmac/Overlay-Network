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

typedef unsigned char byte;

#define MAXFDS  32
#define PACKET  1024
#define PAYLOAD 1022
#define MANAGERPACKET 128

enum DIRECTION {
    UP = 0,
    RIGHT = 1,
    DOWN = 2,
    LEFT = 3
};

enum CONTENTTYPE{
    NEW = 0,
	RESP,
    TALK,
    END 
};

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

class ManagerHandler{
    private:
        unsigned int ip;
		int nb = 0;
        int port = -1;
        int manager = -1;
        int dir;

		pthread_cond_t cond = PTHREAD_COND_INITIALIZER;	
		pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

        byte packet[MANAGERPACKET] = {0};

    public:


		//	  1byte(1)		|     2bytes
		// 	<packet type>	|	<clientID>	-> Register
		//

		//	  1byte(2)		|       1bytes		  |				1byte
		// 	<packet type>	|	 <Type 2/3/4>	  |		<Awaited Connections>	-> Aprovall
		//						<Not accepted 0>

		//    1byte(3)      |      1byte      |   4bytes
		//  <packet type>   |   <direction>   |    <IP>    -> Connect to new node
		//

		//    1byte(4)      |      1byte      |     2bytes
		//  <packet type>   |   <direction>   |   <clientID>  -> Patch up
		//

		//       1byte
		//    <direction>   -> New neighbor
		//


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

		

		void setManagerConf(unsigned int ip, int port){
			this->ip = ip;
            this->port= port;
		}

        int connectToManager(){
			if (manager == -1){
				puts("Connecting...");
				pthread_mutex_lock(&mutex);
				while ((manager = tcpClientSocket(ip, port)) < 0){
					sleep(1);
					puts("waiting");
					//erro - Manager not available
				}
				pthread_cond_signal(&cond);
				pthread_mutex_unlock(&mutex);
			}
			puts("connected");
            return 0;
        }

		int forward(){
			if ((nb = send(manager, packet, MANAGERPACKET, 0)) > 0)
				return nb;
			close(manager);
			manager = -1;
            return -1;//Erro connection closed
        }

        int get(){
            if ((nb = recv(manager, packet, MANAGERPACKET, 0)) > 0){
				return nb;
			}
			close(manager);
			manager = -1;
            return -1;//Erro connection closed
        }

		void disconnected(){
			puts("Adormecer...");
			if (nb == 0){
				pthread_mutex_lock(&mutex);
				pthread_cond_wait(&cond, &mutex);
				pthread_mutex_unlock(&mutex);
			}
			puts("....acordou");
		}

		void buildRegisterPacket(uint16_t id){
			puts("Register");
			memset(packet, 0 , MANAGERPACKET);
            packet[0] = 1;
            packet[1] = id;
			packet[2] = id>>8;
			//certificado e outras conices
		}

		void buildPatchUp(byte dir, byte id){
			printf("O vizinho %d bazou\n", dir);
			memset(packet, 0 , MANAGERPACKET);
			packet[0] = 4;
			packet[1] = dir;
			packet[2] = id;
			packet[3] = id>>8;

		}

	
		//manager answer

		int checkPacket(){
			if (packet[0] > 0 && packet[0] < 5)
				return packet[0];
			return -1;
		}
        int getNodeType(){
			byte n = packet[1];
			if (n == 0 || (n > 1 && n < 5)) 
				return n;
            return -1; //Aproval not granted
        }

        int getConnectingNeighbors(){
			byte n = packet[2];
			if (n >= 0 && n < 5) 
				return n;
            return -1; // invalid awaited connections
        }

		//new node connection
        int getNeighborDir(){
			byte n = packet[1];
			if (n >= 0 && n < 4)
				return n;
            return -1;
        }

        unsigned int getNeighborIp(){
            return (packet[2] | (packet[3] << 8) | (packet[4] << 16) | (packet[5] << 24));
		}
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
byte availableNeighbor[4] = {0};	//  // ////// // ////// //  // /////  ////// /////	//
byte type;							/// // //     // //  	//  // //  // //  // //  //	//
int neighbors [5] = {0}; 			////// /////  // // ///	////// /////  //  // /////	//
//									// /// //     // //  //	//  // //  // //  // // //	//
//									//  // ////// // //////	//  // /////  ////// //  //	//
//Thread control																		//
ThreadPool neighbor;																	//
//////////////////////////////////////////////////////////////////////////////////////////

pthread_t managerSlave;
ManagerHandler manager;
int nodeID = 0;

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
            int i = random(4);
            while(!availableNeighbor[i] || listenDirection == i){
                i = random(4);
            }
            byte dir = i;
            buffer[0] |= (dir & 0x03)<<(hop<<1);
        
            return dir;
        }

		byte getNewPath(byte vector[4]){
            int dir[3] = {0};
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
			byte f, s;
			if(hop == 2){//11 00 00 11
				f = buffer[0]>>4 & 0x03;
				s = buffer[0]>>2 & 0x03;
				buffer[0] &= 0xc3;
				buffer[0] |= f<<2 | s<<4;
				return s;
			}
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
            return n;
        } 
        int forward(){
            n = send(neighbors[direction], buffer, PACKET, 0);
            if (n < 0)
                return -1;
            return n;
        }

        //PACKET PARAMETER RELATED
        short getNextDirection(){
            //   6bit    2bits | 2bits     1bits(0/1)      5bits    |   Rest
            // <circuit> <hop> | <new>    <ip/dominio>   <streamID> | <Payload>

            //                   2bits      1bit(0/1)      5bits    |   Resto
            // <circuit> <hop> | <talk>   <local/exit>   <streamID> | <Payload>
            // <circuit> <hop> | <end>    <local/exit>   <streamID> | errr - qual foi

            // <circuit> <hop> | <resp>      <local>     <streamID> | <Payload>

            byte type = (buffer[1]>>6) & 0x03;//saber se é new ou nao
            byte hops = (buffer[0]) & 0x03;//numero de hops
		
            if (hops == 0){
                switch(type){
                    case NEW:
                        return 1;
					case RESP:
						return 2;
                    case TALK:
                        return 3;
                    case END:
                        return 4;
					
                }
            }else{
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
        
		short getStreamID(){	
			return buffer[1] & 0x1f;
        }
       
	    bool isExitNode(){
            if ((buffer[1]>>5) & 0x01) 
                return true;
            return false;
        }

        byte* getPayload(){		
			return &buffer[2];
        }

        byte getAddrType(){		
			return (buffer[1] & 0x20)>>5;
        }

        byte* getAddr(){ 		
			return &buffer[2];
        }

		bool getResponseState(){
			if ((buffer[1] >> 5) & 0x01)
				return true;
			return false;
		}

        //PACKET CREATION
        void freshBuild(bool domain, short streamID, byte* payload){
            direction = getRandomDirection(3);
            buffer[0] |= 2 & 0x03;

            buffer[1] = 0<<6;
            if (domain){
				buffer[1] |= 1<<5;
				memcpy(&buffer[2], payload, payload[0]+1);
			}else{
				memcpy(&buffer[2], payload, 4);
			}
            buffer[1] |= streamID & 0x1f;
        }

		void buidResponse(short streamID, unsigned int vector, bool success){
			puts("Build response");
			byte circuit = getNewPath((byte*) &vector);
			buffer[0] = circuit;

            buffer[1] = 1<<6;
            if (success)
				buffer[1] |= 1<<5;

            buffer[1] |= streamID & 0x1f;
			getNextDirection();
		}

        void build(short streamID, unsigned int vector, bool exit, byte* payload){
            buffer = payload;

            byte circuit = getNewPath((byte*) &vector);
            buffer[0] = circuit;
            buffer[1] = 2<<6;

            if (exit)
                buffer[1] |= 1<<5;
            buffer[1] |= streamID & 0x1f;
            getNextDirection();
        }

        void endBuild(short streamID, unsigned int vector, bool exit){
			puts("Build test end");
            memset(buffer, 0, PACKET);
			
            byte circuit = getNewPath((byte*) &vector);
            
            buffer[0] = circuit;
            
            buffer[1] = 3<<6;
            
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
void forwardingHandler(int direction){
    int n = 1, streamID, result;
    Protocol packet(direction);

    while (n){
    	if (packet.get() > 0){
			streamID = 0;
		    result = packet.getNextDirection();//caso seja para encaminhar faz aqui a alteracao do salto
		    streamID = packet.getStreamID();
		    pair<unsigned int,short> p(packet.getVector(), streamID);

	        switch(result){
		       	case 0://é para encaminhar    
				   	puts("Forward!!!");
		            packet.forward();
		            break; 
		        case 1:{//new exitSocket
					pair<unsigned int,short> r(invert(p.first), streamID);
					puts((char*) packet.getPayload());
					packet.buidResponse(streamID, r.first, true);
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
				case 2:
					if (packet.getResponseState()){
						puts("Success!!!");
						//if (path[streamID] == 0)
		                //    path[streamID] = invert(packet.getVector());
					}else{
						closeLocal(streamID);
					}
					break;
		        case 3://talk
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
		        case 4://end
					puts("Recebeu resposta");
		            if (packet.isExitNode()){
		                closeExit(p);
		            }else{
		                closeLocal(streamID);
		            }
		            break;
		    }     
		} else {
			manager.buildPatchUp(direction, nodeID);
			close(neighbors[direction]);
			neighbors[direction] = -1;
			availableNeighbor[direction] = 0;
			manager.connectToManager();
			manager.forward();
			n = 0;
		}
    }
    //return NULL;
}

void manageConnections(int nc){
	byte buffer[MANAGERPACKET];

	struct sockaddr_in clientAddr = {0};
    socklen_t addrLen = sizeof(struct sockaddr_in);
	int client = -1, server = tcpServerSocket(9999);
	
	while (nc){
		client = accept(server, (struct sockaddr*) &clientAddr, &addrLen);
		if (client > 0){
			if ((recv(client, buffer, MANAGERPACKET, 0)) > 0){
				if (buffer[0] >= 0 && buffer[0] < 4) {
					printf("Vizinho na %d\n", buffer[0]);
					availableNeighbor[buffer[0]] = 1;
					neighbors[buffer[0]] = client;
					neighbor.sendWork(buffer[0]);
					nc--;
					continue;
				}
			}
		}
		//erro client invalid
		close(client);
	}
	close(server);
}

void* Manager(void* null){

	manager.connectToManager();
	manager.buildRegisterPacket(nodeID);

	manager.forward();
	type = 1;

	while (type){
		while (manager.get() > 0){
			puts("Mensagem do manager");
			switch (manager.checkPacket()){
				case 2:{
					type = manager.getNodeType();
					printf("Tipo: %d\n", type);
					if (type){
						int nc = manager.getConnectingNeighbors();
						printf("Espero %d ligacoes\n", nc);
						if (nc)
							manageConnections(nc);
					}else{
						printf("Bye\n");
						exit(0);
					}
					break;
				}
				case 3:{
					int dir = manager.getNeighborDir();
					if (dir != -1){
						int d = invertDir(dir);
						sleep(1);
						if ((neighbors[d] = tcpClientSocket(manager.getNeighborIp(), 9999)) > 0){
							availableNeighbor[d] = 1;
							printf("Ligado ao vizinho na %d\n", d);
							send(neighbors[d], &dir, 1, 0);
							neighbor.sendWork(d);
						}
						//neighor inexistent
					}
					//erro dir invalid
					break;
				}
				default:
					//unknown packet
					break;
			}

		}
		manager.disconnected();
	}
	return NULL;
}

int main(int argc, char const *argv[]){
	if (argc < 2){
		//erro - argvaaa
		return 0;
	}
	
	neighbor.init(4, (void*) &neighbor, forwardingHandler);

	nodeID = atoi(argv[1]);
	unsigned int managerAddr = 0;

	inet_pton(AF_INET, "10.0.0.10", &managerAddr);
	manager.setManagerConf(managerAddr, 7777);
	pthread_create(&managerSlave, NULL, Manager, NULL);
	pthread_detach(managerSlave);

	byte buffer[PACKET];
	
	
	Protocol p(-1);	
	while(1){
		memset(buffer,0,PACKET);
		read(0, &buffer[2], PAYLOAD);
		p.buildTest(buffer);
		p.forward();
	}

	return 0;
}