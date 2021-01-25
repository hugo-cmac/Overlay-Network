#include <cstdio>
#include <iostream>
#include <map>

#include <fcntl.h>
 #include <signal.h>

#include "clientprotocol.h"
#include "criptography.h"
#include "managerprotocol.h"
#include "socks.h"
#include "tcp.h"
#include "threadpool.h"


using namespace std;


//////////////////////////////////////////////////////////////////////////////////////////
// Exit Sockets                                                                     	//
int  exitSocks [MAXFDS] = {0};                                            				//
// < < vector, streamID >, streamID>                    	   ////// //  // // /////// //
map<pair<unsigned int, int>, int> streamIdentifier; 	   //      ////  //   //    //
//                                                      	   /////    //   //   //    //
// < streamID, < vector, streamID >                     	   //      ////  //   //    //
map<int, pair<unsigned int, int>> pathIdentifier;   	   ////// //  // //   //    //
// Thread control																		//
ThreadPool exitSlaves;																	//
//////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////
//                                                                                 		//
// Path to exit node                            	 //     ////// //////  ////  //     //
unsigned int path[MAXFDS] = {0};                	 //     //  // //     //  // //     //
//                                              	 //     //  // //     ////// //     //
// Local Sockets                                	 //     //  // //     //  // //     //
int localSocks [MAXFDS] = {0};        				 ////// ////// ////// //  // ////// //
//Thread control																		//
ThreadPool localSlaves;																    //
//////////////////////////////////////////////////////////////////////////////////////////

ThreadPool neighborSlaves;

Criptography* crypto[4];

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t printmutex = PTHREAD_MUTEX_INITIALIZER;


pthread_t managerSlave;
ManagerProtocol manager;
int nodeID = 0;

void concurrencyPrint(char *b){
	pthread_mutex_lock(&printmutex);
	puts(b);
	pthread_mutex_unlock(&printmutex);
}

int webConnect(byte* socksPacket){
	char addr[255];
	uint32_t ipv4 = 0;
	byte ipv6[16];
	uint16_t port = 0;
	int sock = -1;
	int opt = -1;

	char b[300];

	if(socksPacket[0] == 0x04){
		ipv4 = (socksPacket[4]) | (socksPacket[5] << 8) | (socksPacket[6] << 16) | (socksPacket[7] << 24);
		port = (socksPacket[2] << 8) | (socksPacket[3]);
		socksPacket[1] = 0x5A;
		opt = 1;
	}else if(socksPacket[0] == 0x05){
		socksPacket[1] = 0x00;
		switch (socksPacket[3]){
			case 0x01: // ipv4
				ipv4 = (socksPacket[4]) | (socksPacket[5] << 8) | (socksPacket[6] << 16) | (socksPacket[7] << 24);
				port = (socksPacket[8] << 8) | (socksPacket[9]);
				sprintf(b, "IPv4 = %d.%d.%d.%d:%d\n",socksPacket[4], socksPacket[5] << 8, socksPacket[6] << 16, socksPacket[7] << 24, port);
				concurrencyPrint(b);
				opt = 1;
				break;

			case 0x03: // domain
				memset(addr, '\0' , 255);
				memcpy(addr, &socksPacket[5], socksPacket[4]);
				port = (socksPacket[socksPacket[4]+5] << 8) | socksPacket[socksPacket[4]+6];
				sprintf(b, "Domain = %s:%d\n", addr, port);
				concurrencyPrint(b);
				opt = 2;
				break;

			case 0x04: // ipv6
				memcpy(ipv6, &socksPacket[4], 16);
				port = (socksPacket[20] << 8) | (socksPacket[21]);
				printf("IPv6 = ");
				for (size_t i = 0; i < 16; i++){
					printf("%02x%02x.", socksPacket[i], socksPacket[++i]);
				}
				printf(":%d\n", port);
				opt = 3;
				break;
		}
	}else{
		sprintf(b, "NO VERSION NO ADDRESS\n");
		concurrencyPrint(b);
		return -1;
	}

	switch (opt){
		case 1:
			sock = tcpClientSocket(ipv4, port);
			return sock;
		case 2:{
			char portchar[6];
			snprintf(portchar, 6, "%d", port);
			
			struct addrinfo *result, *rp;
			int r = getaddrinfo(addr, portchar, NULL, &result);

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
				freeaddrinfo(result);
			}
			break;
		}
		case 3:
			sock = tcpClientSocketIPv6(ipv6, port);
			return sock;
	}

    return -1;
}

int findEmpty(int* list, int fd){
    short i = -1;
	pthread_mutex_lock(&mutex);
    while (i++ < MAXFDS){
		printf("e");
        if (list[i] == -1){   
            list[i] = fd;
			pthread_mutex_unlock(&mutex);
            return i;
        }
    }
	pthread_mutex_unlock(&mutex);
    return -1;
}

void closeExit(pair<unsigned int,short> p){
	pthread_mutex_lock(&mutex);
    short streamID = streamIdentifier[p];
		
    shutdown(exitSocks[streamID], SHUT_RDWR);
	close(exitSocks[streamID]);
    
	exitSocks[streamID] = -1;
    
	streamIdentifier.erase(p);
    pathIdentifier.erase(streamID);
	pthread_mutex_unlock(&mutex);
}

void closeLocal(short streamID){
	pthread_mutex_lock(&mutex);
	if (localSocks[streamID] != -1){
		shutdown(localSocks[streamID], SHUT_RDWR);
		close(localSocks[streamID]);

		localSocks[streamID] = -1;
		
		path[streamID] = 0;
	}
	pthread_mutex_unlock(&mutex);
}

int readFromNetwork(ClientProtocol* packet){
	// byte* buffer = packet->read();
	// if (buffer == NULL)
	// 	return -1;
	
	// //desencriptar
	// //setbuffer
	return 10;
}

int writeToNetwork(ClientProtocol* packet, byte* buffer){
	// if (buffer == NULL)
	// 	return -1;
	// //encriptar
	// if (packet->write(buffer) > 0){
	// 	return 10;
	// }

	return -1;
}

void exitNodeHandler(int streamID){
	pair<unsigned int, int> p = pathIdentifier[streamID];
	
	ClientProtocol packet(6);
    byte* buffer = (byte*)malloc(PACKET);
	int n = 1;
    char b[300];
	sprintf(b, "OPEN THREAD: exitSocks[%d] = [%d]", p.second, exitSocks[streamID]);
	concurrencyPrint(b);

	byte circuit = p.first;
    while (n){
		putchar('f');
    	memset(buffer, 0, PACKET);
		n = recv(exitSocks[streamID], &buffer[4], PAYLOAD, 0);
		if (n > 0) {
			/////////////////////////////////////////////
			packet.buildTalk(p.second, circuit, false, buffer, n);
			if (!(packet.write() > 0)){
				sprintf(b,"ERRO: writing to neighbor");
				concurrencyPrint(b);
			}
			/////////////////////////////////////////////
		} else {
			sprintf(b, "END THREAD: exitSocks[%d] = [%d]", p.second, exitSocks[streamID]);
			concurrencyPrint(b);
			if (exitSocks[streamID] != -1){
				closeExit(p);

				/////////////////////////////////////////////
				packet.buildEnd(streamID, circuit, false);
				packet.write();
				/////////////////////////////////////////////
			}
			return;
		}
    }
}

void proxyLocalHandler(int streamID){
	ClientProtocol packet(6);
	byte* buffer = (byte*)malloc(PACKET);
	int n = 1;

	char b[300];
	sprintf(b, "OPEN THREAD : localSocks[%d] = [%d]", streamID, localSocks[streamID]);
	concurrencyPrint(b);

	byte circuit = path[streamID];

	while(n){
		putchar('g');
		memset(buffer, 0, PACKET);
		n = recv(localSocks[streamID], &buffer[4], PAYLOAD, 0);
		if (n > 0){
			/////////////////////////////////////////////
			packet.buildTalk(streamID, circuit, true, buffer, n);

			if (!(packet.write() > 0)){
				sprintf(b, "ERRO: writing to neighbor");
				concurrencyPrint(b);
			}
			
			/////////////////////////////////////////////
		}else{
			sprintf(b, "END THREAD: localSocks[%d] = [%d]", streamID, localSocks[streamID]);
			concurrencyPrint(b);
			if (localSocks[streamID] != -1){
				closeLocal(streamID);

				/////////////////////////////////////////////
				packet.buildEnd(streamID, circuit, true);
				packet.write();
				/////////////////////////////////////////////
			}
			return;
		}
	}
}

void proxyServerProcedure(int proxyClient){

    Socks client(proxyClient);
	ClientProtocol packet(6);
	char b[300];

    if (client.handleGreetingPacket()) {
    	if (client.handleConnectionReq()) { // se for dominio ou ipv4 retorna true
			short streamID = findEmpty(localSocks, proxyClient);
			if (streamID != -1){
				sprintf(b, "NEW: localSocks[%d] = [%d]", streamID, localSocks[streamID]);
				concurrencyPrint(b);
				/////////////////////////////////////////////
	    		packet.buildNew(streamID, client.getPayload(), client.getSize());
				packet.write();
				/////////////////////////////////////////////
				return;
	    	}	    
    	}
    }
    close(proxyClient);
}

void proxyServerHandler(){

	struct sockaddr_in addr;
    socklen_t len = sizeof(struct sockaddr_in);
	int one = 1, server = -1, client = 0;
    while (server < 0){
		putchar('h');
    	sleep(1);
    	server = tcpServerSocket(9999);
    }

    while (1){
		putchar('i');
        client = accept(server, (struct sockaddr*) &addr, &len);
        if (client < 0) {
            close(client);
        } else {
			//setsockopt(server, SOL_TCP, TCP_NODELAY, &one, sizeof(one));
        	//setsockopt(client, SOL_TCP, TCP_NODELAY, &one, sizeof(one));
			proxyServerProcedure(client);
        }
    }

}

void forwardingHandler(int direction){
    int n = 1, streamID, result;
	byte* payload;
	pair<unsigned int,short> p;

    ClientProtocol packet(direction);
	char b[300];

	for(int x = 0 ; x < 4; x++){
		printf("neighbor[%d] = %d\n", x, availableNeighbor[x]);
	}

    while (n){
		putchar('j');
    	if (packet.read() > 0){
			streamID = 0;
		    result = packet.getNextDirection();//caso seja para encaminhar faz aqui a alteracao do salto
		    streamID = packet.getStreamID();
			payload = packet.getPayload();
			p = make_pair(packet.getCircuit(), streamID);

	        switch(result){
		       	case 0:{//é para encaminhar  
		            packet.write();
		            break;
				}
		        case 1:{//new exitSocket
					pair<unsigned int,short> r(invertCircuit(p.first), streamID);
		            int webSocket = webConnect(payload);
		            if (webSocket > 0){
                        if ((streamIdentifier[p] = findEmpty(exitSocks, webSocket)) != -1){
                            pathIdentifier[streamIdentifier[p]] = r;

							sprintf(b, "NEW: exitSocks[%d] = [%d]", p.second, exitSocks[streamIdentifier[p]]);
							concurrencyPrint(b);

							/////////////////////////////////////////////
                    		packet.buildResponse(streamID, r.first, true);
                    		packet.write();
							/////////////////////////////////////////////
							
							exitSlaves.sendWork(streamIdentifier[p]);
                            break;
                        }
                        close(webSocket);
                        streamIdentifier.erase(p);
		            }
					sprintf(b, "ERRO WEBCONNECT");
					concurrencyPrint(b);

					/////////////////////////////////////////////
					packet.buildResponse(streamID, r.first, false);
					packet.write();
					/////////////////////////////////////////////
		            break;
		        }
				case 2:{
					int ns = 0;
					sprintf(b, "RESPONSE %d: localSocks[%d] = [%d]", packet.getResponseState(), streamID, localSocks[streamID]);
					concurrencyPrint(b);

					if (packet.getResponseState()){
						ns = send(localSocks[streamID], payload, packet.getPayloadSize(), 0);
						if (ns > 0){
							path[streamID] = invertCircuit(p.first);
							localSlaves.sendWork(streamID);
							break;
						}
					}
					
					closeLocal(streamID);
					break;
				}
		        case 3:{//talk
					int ns = 0;
		            if (packet.isExitNode()){//exit sock
						if (exitSocks[streamIdentifier[p]] != -1){
							ns = send(exitSocks[streamIdentifier[p]], payload, packet.getPayloadSize(), 0);
							if(!(ns > 0)){
								sprintf(b, "ERRO TALK: exitSocks[%d] = [%d]", p.second, exitSocks[streamIdentifier[p]]);
								concurrencyPrint(b);
								shutdown(exitSocks[streamIdentifier[p]], SHUT_RDWR);
								close(exitSocks[streamIdentifier[p]]);
							}	
						}
		            }else{//localsock
						if(localSocks[streamID] != -1){
							ns = send(localSocks[streamID], payload, packet.getPayloadSize(), 0);
							if(!(ns > 0)){
								sprintf(b, "ERRO TALK: localSocks[%d] = [%d]", streamID, localSocks[streamID]);
								concurrencyPrint(b);
								shutdown(localSocks[streamID], SHUT_RDWR);
								close(localSocks[streamID]);
							}	
						}
		            }
		            break;
				}
		        case 4:{//end
		            if (packet.isExitNode()){
						if (exitSocks[streamIdentifier[p]] != -1){
							sprintf(b, "CLOSE END: exitSocks[%d] = [%d]", p.second, exitSocks[streamIdentifier[p]]);
							concurrencyPrint(b);
		                	closeExit(p);
						}
		            }else{
						if(localSocks[streamID] != -1){
							sprintf(b, "CLOSE END: localSocks[%d] = [%d]", streamID, localSocks[streamID]);
							concurrencyPrint(b);
							closeLocal(streamID);
						}
					}	
		            break;
				}
		    }     
		} else {
			sprintf(b, "ERRO: neighbor[%d] = [%d]", direction, neighbors[direction]);
			concurrencyPrint(b);
			close(neighbors[direction]);

			neighbors[direction] = -1;
			availableNeighbor[direction] = 0;
			
			//Crypto==0
			//free keys
			
			manager.connectToManager();

			manager.buildPatchUp(direction, nodeID);
			manager.write();

			n = 0;
		}
    }
}

void manageConnections(int nc){
	byte buffer[MANAGERPACKET];

	struct sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(struct sockaddr_in);
	int client = -1, server = tcpServerSocket(8888);
	
	while (nc){
		putchar('k');
		client = accept(server, (struct sockaddr*) &clientAddr, &addrLen);
		if (client > 0){
			if ((recv(client, buffer, MANAGERPACKET, 0)) > 0){
				if (buffer[0] < 4) {
					availableNeighbor[buffer[0]] = 1;
					neighbors[buffer[0]] = client;

					//crypto
					//send public

					neighborSlaves.sendWork(buffer[0]);
					nc--;
					continue;
				}
			}
            close(client);
		}
		//erro client invalid
	}
	close(server);
}

void* Manager(void* null){
    int nc = 0;

	manager.connectToManager();

	manager.buildRegisterPacket(nodeID);
	manager.write();

	type = 1;

	while (type){
		putchar('l');
		while (manager.read() > 0){
			putchar('m');
			switch (manager.checkPacket()){
				case 2:{
					type = manager.getNodeType();
					if (type){
						if ((nc = manager.getConnectingNeighbors()))
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
						int d = invertDirection(dir);
						sleep(1);
						if ((neighbors[d] = tcpClientSocket(manager.getNeighborIp(), 8888)) > 0){
							//getpublic and send
							availableNeighbor[d] = 1;
							send(neighbors[d], &dir, 1, 0);

							//recv pub
							//derive
							neighborSlaves.sendWork(d);
						}
						//neighbor inexistent
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
	return null;
}

void handler(int sig){
	return;
}


int main(int argc, char const *argv[]){
	signal(SIGPIPE, handler);
	if (argc < 2){
        //erro - argvaaa
        return 0;
    }
    timeR = time(NULL);
	memset(localSocks, -1, sizeof(int) * MAXFDS);
	memset(exitSocks, -1, sizeof(int) * MAXFDS);
	
    neighborSlaves.init(4, (void*) &neighborSlaves, forwardingHandler);
    localSlaves.init(MAXFDS, (void*) &localSlaves, proxyLocalHandler);
    exitSlaves.init(MAXFDS, (void*) &exitSlaves, exitNodeHandler);
    
    nodeID = atoi(argv[1]);
	unsigned int managerAddr = 0;

    //inet_pton(AF_INET, "10.0.0.22", &managerAddr);
	inet_pton(AF_INET, "192.168.122.119", &managerAddr);
	manager.setManagerConf(managerAddr, 7777);
	pthread_create(&managerSlave, NULL, Manager, NULL);
	pthread_detach(managerSlave);

	proxyServerHandler();

    return 0;
}
