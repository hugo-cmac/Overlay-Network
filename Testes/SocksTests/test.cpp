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

typedef unsigned char byte;

using namespace std;

#define MAXFDS 32
#define PACKET 1500

pthread_mutex_t tranca;

//////////////////////////////////////////////////////////////////////////////////////
// Exit Sockets                                                                     //
int  exitSocks [MAXFDS] = {0};                                            			//
// < < vector, streamID >, streamID>                    //////  //  //  // ///////  //
map<pair<unsigned int, short>, short> streamIdentifier; //       ////   //   //     //
//                                                      /////     //    //   //     //
// < streamID, < vector, streamID >                     //       ////   //   //     //
map<short, pair<unsigned int, short>> pathIdentifier;   //////  //  //  //   //     //																//
//////////////////////////////////////////////////////////////////////////////////////

pthread_t slave[MAXFDS][2];
int slaveLock[MAXFDS];

//////////////////////////////////////////////////////////////////////////////////////
//                                                                                  //
// Path to exit node                            //     ////// //////  ////  //      //
unsigned int path[MAXFDS] = {0};                //     //  // //     //  // //      //
//                                              //     //  // //     ////// //      //
// Local Sockets                                //     //  // //     //  // //      //
int localSocks [MAXFDS] = {0};        			////// ////// ////// //  // //////  //
//////////////////////////////////////////////////////////////////////////////////////

int proxyServer = -1;


short findEmpty(int* l1, int* l2, int fd){
    short i = -1;
    while (++i < MAXFDS){
        if (l1[i] == -1 && l2[i] == -1){
            l1[i] = fd;
            return i;
        }
    }
    return -1;
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

void printBytes(byte* buffer, int size){
	for (int i = 0; i < size; ++i)
		printf("[%02X]",buffer[i]);
	printf("\n");
}

class Socks{
    private:

    	byte version = 0;
    	
    	int nread = -1;

    	byte buffer[300] = {0};

    	unsigned short bufferSize = 300;

    	byte addr_type = 0;

    	byte* addr = NULL;

        int fd;

        void greetingsAnswer(bool auth){
            byte answer[2] = {0x05, 0xff};
            if(auth){ // method accepted
                answer[1] = 0x00;
            }
            send(fd, answer, 2, 0);
        }

    public:
        Socks(int fd){
            this->fd = fd;
        }

        bool handleGreetingPacket(){ //return false = erro / true = autenticacao e suportada
            nread = recv(fd, buffer, bufferSize, 0);
        	
            if(nread > 0){
            	version = buffer[0];

            	switch(version){
            		case 0x05:
            			if((int)buffer[1] < 1){
            				// nao suporta nenhum tipo de autenticação
                        	greetingsAnswer(false);
            			} else {
            				for(int i = 2 ; i < nread; i++){
	                            if(buffer[i] == 0x00){
	                                greetingsAnswer(true);
	                                return true;
	                            }
                        	}
	                        //nao suporta o nosso tipo de autenticacao
	                        greetingsAnswer(false);
            			}
            			break;
            		case 0x04:
            			// tratar socksv4
            			break;
            		default:
            			// erro
            			return false;
            	}
            }
            return false;
        }
        
        bool handleConnectionReq(){ // Ler pacote Client connection request
	        nread = recv(fd, buffer, bufferSize, 0);
            if(nread > 0){
            	if (version == 0x05){
            		addr_type = buffer[3];
            		if (addr_type == 0x01 || addr_type == 0x03){
            			addr = &buffer[4];
            			return true;
            		}
            	}
            }
            //nao suportado/ erro
            return false;
        }

        int responsePacket(){
        	buffer[1] = 0x00;

        	return send(fd, buffer, nread, 0);
        }

        byte* getAddr(){
            return addr;
        }

        byte getAddrType(){
        	if (addr_type == 0x01){ //ipv4
        		return 0;
        	}
        	return 1;
        }
};

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

void closeStreamID(short streamID){
	close(localSocks[streamID]);	localSocks[streamID] = -1;
	close(exitSocks[streamID]);		exitSocks[streamID] = -1;
}

void printLocalSock(int streamID, int sock){
	char buffer[100];
	sprintf(buffer, "\x1b[0;0f\x1b[%d;22f%d ", streamID+1, sock);
	puts(buffer);
}
void printLocalThread(int streamID, int thread){
	char buffer[100];
	sprintf(buffer, "\x1b[0;0f\x1b[%d;36f%d ", streamID+1, thread);
	puts(buffer);
}
void printExitSock(int streamID, int sock){
	char buffer[100];
	sprintf(buffer, "\x1b[0;0f\x1b[%d;49f%d ", streamID+1, sock);
	puts(buffer);
}
void printExitThread(int streamID, int thread){
	char buffer[100];
	sprintf(buffer, "\x1b[0;0f\x1b[%d;63f%d ", streamID+1, thread);
	puts(buffer);
}

void closeStream(short streamID, int peer_id, int id){
	int n = 0;
	printLocalSock(streamID, localSocks[streamID]);
	if (localSocks[streamID] != -1){
		shutdown(localSocks[streamID], SHUT_RDWR);
		localSocks[streamID] = -1;
	}

	if (exitSocks[streamID] != -1){
		shutdown(exitSocks[streamID], SHUT_RDWR);
		exitSocks[streamID] = -1;
	}
	printExitSock(streamID, exitSocks[streamID]);


	n = pthread_cancel(slave[streamID][peer_id]);

	if (id){
		printLocalThread(streamID, n);
		printExitThread(streamID, 0);
	}else{
		printExitThread(streamID, n);
		printLocalThread(streamID, 0);
	}
	
	pthread_exit(0);
	if (id){
		printExitThread(streamID, 0);
	}else{
		printLocalThread(streamID, 0);
	}
}


void *exitNodeHandler(void* s){
	
	short streamID = *(short*)(s);

	byte* buffer = (byte*)malloc(PACKET);
	int n = 1;
    
	while(n){
		n = recv(exitSocks[streamID], buffer, PACKET, 0);
		
		if (n > 0){
			n = send(localSocks[streamID], buffer, n, 0);
		} else {
			closeStream(streamID, 0, 1);
		}			
	}
	return NULL;
}


void* proxyLocalHandler(void* s){

	short streamID = *(short*)(s);

	byte* buffer = (byte*)malloc(PACKET);
	int n = 1;
    
	while(n){
		n = recv(localSocks[streamID], buffer, PACKET, 0);
		if (n > 0) {
			n = send(exitSocks[streamID], buffer, n, 0);
		} else {
			closeStream(streamID, 1, 0);
		}		
	}
	return NULL;
}


void proxyServerProcedure(int proxyClient){

    Socks clientsock(proxyClient);

    if (clientsock.handleGreetingPacket()) {
		
    	if (clientsock.handleConnectionReq()) { // se for dominio ou ipv4 retorna true
    		byte* addr = clientsock.getAddr();
    		byte type = clientsock.getAddrType();

	    	if (clientsock.responsePacket() > 0) {

	    		short streamID = findEmpty(localSocks, exitSocks, proxyClient);
	    		if (streamID >= 0){
	    			int exitSocket = webConnect(type, addr);
	    			if(exitSocket > 0){
	    				printLocalSock(streamID, proxyClient);
	    				printExitSock(streamID, exitSocket);

	    				exitSocks[streamID] = exitSocket;

						pthread_create(&slave[streamID][0], NULL, proxyLocalHandler, (void*)(&streamID));
						printLocalThread(streamID, 1);
						
						pthread_create(&slave[streamID][1], NULL, exitNodeHandler,   (void*)(&streamID));
						printExitThread(streamID, 1);

		    			return;
	    			}
	    			closeStreamID(streamID);
	    		}
	    	}
    	}
    }
    close(proxyClient);
}

void end(int s){
	sleep(10);
	short i = -1;
	while(++i < MAXFDS){
		if (localSocks[i] != -1)
			close(localSocks[i]);
		if (exitSocks[i] != -1)
			close(exitSocks[i]);
	}
	puts("\x1b[34;0f>> End                  \n");
	close(proxyServer);
	exit(0);
}

//threadA
void* proxyServerHandler(void* thread){
	signal(SIGINT, end);
	int one = 1;
    while (proxyServer < 0){
    	sleep(1);
    	proxyServer = tcpServerSocket(9999);
    }
    puts("\x1b[34;0f>> Ready                     \n");
    
    while (1){
        struct sockaddr_in client = {0};
        socklen_t len = sizeof(struct sockaddr_in);
        int proxyClient = accept(proxyServer, (struct sockaddr*) &client, (socklen_t *)&len);
      	setsockopt(proxyServer, SOL_TCP, TCP_NODELAY, &one, sizeof(one));
        if (proxyClient < 0){
            close(proxyClient);
        } else {
        	setsockopt(proxyClient, SOL_TCP, TCP_NODELAY, &one, sizeof(one));
        	
        	proxyServerProcedure(proxyClient);
        }
    }
    pthread_exit(NULL);
}

void setupSockets(){
	memset(localSocks, -1, MAXFDS*sizeof(int));
	memset(exitSocks, -1, MAXFDS*sizeof(int));
}

void setupTerminal(){
	printf("\x1b[2J\x1b[0;0f");
	for (int i = 0; i < MAXFDS; ++i){
		if (i > 9){
			printf("Stream[%d]: [Local = -1   Thread = 0 ]  [Exit = -1   Thread = 0 ]\n", i);
		}else{
			printf("Stream[%d] : [Local = -1   Thread = 0 ]  [Exit = -1   Thread = 0 ]\n", i);
		}
	}
}

int main(int argc, char const *argv[]){
	setupSockets();
	
	setupTerminal();
	
	proxyServerHandler((void *) NULL);

	return 0;
}