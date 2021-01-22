#include <cstdio>
#include <iostream>
#include <map>

#include <openssl/aes.h>
#include <openssl/ecdh.h>
#include <openssl/ec.h>
#include <openssl/pem.h>

#include "clientprotocol.h"
#include "managerprotocol.h"
#include "socks.h"
#include "tcp.h"
#include "threadpool.h"


using namespace std;

//////////////////////////////////////////////////////////////////////////////////////////
// Exit Sockets                                                                     	//
int  exitSocks [MAXFDS] = {0};                                            				//
// < < vector, streamID >, streamID>                    	   ////// //  // // /////// //
map<pair<unsigned int, short>, short> streamIdentifier; 	   //      ////  //   //    //
//                                                      	   /////    //   //   //    //
// < streamID, < vector, streamID >                     	   //      ////  //   //    //
map<short, pair<unsigned int, short>> pathIdentifier;   	   ////// //  // //   //    //
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

pthread_t managerSlave;
ManagerProtocol manager;
int nodeID = 0;

class Criptography{
    private:
        AES_KEY ekey;
        AES_KEY dkey;

        unsigned char* skey = NULL;

        EVP_PKEY *privateKey = NULL;
        EVP_PKEY *publicKey = NULL;


        EVP_PKEY* generateKey(){
            EVP_PKEY_CTX *paramGenCtx = NULL, *keyGenCtx = NULL;
            EVP_PKEY *params= NULL, *keyPair= NULL;

            paramGenCtx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL);

            if(EVP_PKEY_paramgen_init(paramGenCtx)){
                EVP_PKEY_CTX_set_ec_paramgen_curve_nid(paramGenCtx, NID_X9_62_prime256v1);
                EVP_PKEY_paramgen(paramGenCtx, &params);

                keyGenCtx = EVP_PKEY_CTX_new(params, NULL);

                if(EVP_PKEY_keygen_init(keyGenCtx)){
                    if(EVP_PKEY_keygen(keyGenCtx, &keyPair)){
                        EVP_PKEY_CTX_free(paramGenCtx);
                        EVP_PKEY_CTX_free(keyGenCtx);
                        return keyPair;
                    }
                } 
            }
            EVP_PKEY_CTX_free(paramGenCtx);
            EVP_PKEY_CTX_free(keyGenCtx);
            return NULL;
        }

        EVP_PKEY* extractPublicKey(EVP_PKEY *privateKey){
            EC_KEY* ecKey = EVP_PKEY_get1_EC_KEY(privateKey);
            EC_POINT* ecPoint = (EC_POINT*) EC_KEY_get0_public_key(ecKey);

            EVP_PKEY *publicKey = EVP_PKEY_new();

            EC_KEY *pubEcKey = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);

            EC_KEY_set_public_key(pubEcKey, ecPoint);

            EVP_PKEY_set1_EC_KEY(publicKey, pubEcKey);

            EC_KEY_free(ecKey);
            EC_POINT_free(ecPoint);

            return publicKey;
        }

    public:
        int publicKeySize = 0;
        size_t sharedKeySize = 0;

        Criptography(){
            privateKey = generateKey();
            publicKey = extractPublicKey(privateKey);
        }
        
		//return pk pointer
        unsigned char* getPublicKey(){
            unsigned char* pk = NULL;
            publicKeySize = i2d_PUBKEY(publicKey, &pk);
            return pk;
        }

        int deriveShared(unsigned char *publicKey){
            EVP_PKEY_CTX *derivationCtx = NULL;
        
            EVP_PKEY *pub = d2i_PUBKEY( NULL, (const unsigned char **) &publicKey, publicKeySize);

            derivationCtx = EVP_PKEY_CTX_new(privateKey, NULL);

            EVP_PKEY_derive_init(derivationCtx);
            EVP_PKEY_derive_set_peer(derivationCtx, pub);

            if(EVP_PKEY_derive(derivationCtx, NULL, &sharedKeySize)){
                if((skey = (unsigned char*) OPENSSL_malloc(sharedKeySize)) != NULL){
                    if((EVP_PKEY_derive(derivationCtx, skey, &sharedKeySize))){
                        AES_set_encrypt_key(skey, sharedKeySize*8, &ekey);
                        AES_set_decrypt_key(skey, sharedKeySize*8, &dkey);
                        EVP_PKEY_CTX_free(derivationCtx);
                        return 1;
                    }
                }
            }
            EVP_PKEY_CTX_free(derivationCtx);
            return 0;
        }

        unsigned char* getSharedKey(){
            return skey;
        }

        //pt +1 e len +1
        void encript(unsigned char* pt, int len){
            unsigned char ct[len];
            unsigned char iv[AES_BLOCK_SIZE];
		    memset(iv, 0, AES_BLOCK_SIZE);

            AES_cbc_encrypt(pt, ct, len, &ekey, iv, AES_ENCRYPT);
            memcpy(pt, ct, len);
        }

        //pt +1 e len +1
        void decript(unsigned char* ct, int len){
            unsigned char pt[len];
            unsigned char iv[AES_BLOCK_SIZE];
		    memset(iv, 0, AES_BLOCK_SIZE);

            AES_cbc_encrypt(ct, pt, len, &dkey, iv, AES_DECRYPT);
            memcpy(ct, pt, len);
        }
    
};

int webConnect(byte* socksPacket){
	char addr[255];
	uint32_t ipv4 = 0;
	byte ipv6[16];
	uint16_t port = 0;
	int sock = -1;
	int opt = -1;

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
				opt = 1;
				break;

			case 0x03: // domain
				memset(addr, '\0' , 255);
				memcpy(addr, &socksPacket[5], socksPacket[4]);
				port = (socksPacket[socksPacket[4]+5] << 8) | socksPacket[socksPacket[4]+6];
				opt = 2;
				break;

			case 0x04: // ipv6
				memcpy(ipv6, &socksPacket[4], 16);
				port = (socksPacket[20] << 8) | (socksPacket[21]);
				opt = 3;
				break;
		}
	}

	switch (opt){
		case 1:
			printf("| ipv4: %d.%d.%d.%d", socksPacket[4], socksPacket[5], socksPacket[6], socksPacket[7]);
			printf(":port: %d\n", port);
			sock = tcpClientSocket(ipv4, port);
			return sock;
		case 2:{
			printf("Domain: %s", addr);
			char portchar[6];
			snprintf(portchar, 6, "%d", port);
			printf(":%s\n", portchar);
			
			struct addrinfo *result, *rp;
			int r = getaddrinfo(addr, portchar, NULL, &result);

			if (r == 0){
				for (rp = result; rp != NULL; rp = rp->ai_next) {
					sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
					if (sock > 0) {
						r = connect(sock, rp->ai_addr, rp->ai_addrlen);
						if (r == 0) {
							printf("Connected: sock %d\n", sock);
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

void exitNodeHandler(int streamID){
	ClientProtocol packet(6);
    byte* buffer = (byte*)malloc(PACKET);
	int n = 1;
    pair<unsigned int, short> p = pathIdentifier[streamID];
	byte circuit = packet.getNewPath((byte*)&p.first);
	printf("Exit inicializado %d\n", streamID);
    while (n){
    	memset(buffer, 0, PACKET);
		n = recv(exitSocks[streamID], &buffer[4], PAYLOAD, 0);
		if (n > 0) {
			packet.buildTalk(p.second, circuit, false, buffer, n);
			packet.write();
		} else {
			printf("ExitHandler leu %d\n", n);
			packet.buildEnd(streamID, circuit, false);
            packet.write();
			printf("A fechar LOCAL\n");
			closeExit(p);
			return;
		}
    }
}

void proxyLocalHandler(int streamID){
	ClientProtocol packet(6);
	byte* buffer = (byte*)malloc(PACKET);
	int n = 1;

    printf("Local inicializado %d\n", streamID);
	byte circuit = packet.getNewPath((byte*)&path[streamID]);
	while(n){
		memset(buffer, 0, PACKET);
		n = recv(localSocks[streamID], &buffer[4], PAYLOAD, 0);
		if (n > 0){
			packet.buildTalk(streamID, circuit, true, buffer, n);
			packet.write();
		}else{
			packet.buildEnd(streamID, circuit, true);
            packet.write();
			printf("A fechar LOCAL\n");
			closeLocal(streamID);
			return;
		}
		//sleep(1);
	}
}

void proxyServerProcedure(int proxyClient){

    Socks client(proxyClient);
	ClientProtocol packet(6);

    if (client.handleGreetingPacket()) {
    	if (client.handleConnectionReq()) { // se for dominio ou ipv4 retorna true
			short streamID = findEmpty(localSocks, proxyClient);
			if (streamID != -1){
	    		packet.buildNew(type, streamID, client.getPayload(), client.getSize());
				packet.write();
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
    	sleep(1);
    	server = tcpServerSocket(9999);
    }

    while (1){
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
    ClientProtocol packet(direction);
	for(int x = 0 ; x < 4; x++){
		printf("neighbor[%d] = %d\n", x, availableNeighbor[x]);
	}

    while (n){
    	if (packet.read() > 0){
			streamID = 0;
		    result = packet.getNextDirection();//caso seja para encaminhar faz aqui a alteracao do salto
		    streamID = packet.getStreamID();
			byte *payload = packet.getPayload();

		    pair<unsigned int,short> p(packet.getVector(), streamID);

	        switch(result){
		       	case 0://é para encaminhar  
				   	printf("Encaminhou, vindo da %d\n", direction);
		            packet.write();
		            break; 
		        case 1:{//new exitSocket
					pair<unsigned int,short> r(invertVector(p.first), streamID);
		            int webSocket = webConnect(payload);
		            if (webSocket > 0){
                        if ((streamIdentifier[p] = findEmpty(exitSocks, webSocket)) != -1){
							printf("Abriu nova exit %d\n", streamIdentifier[p]);
                            pathIdentifier[streamIdentifier[p]] = r;
                    		packet.buildResponse(streamID, r.first, true);
                    		packet.write();
                            exitSlaves.sendWork(streamIdentifier[p]);
                            break;
                        }
						puts("Nao devias passar aqui");
                        close(webSocket);
                        streamIdentifier.erase(p);
		            }

					packet.buildResponse(streamID, r.first, false);
					packet.write();
		            break;
		        }
				case 2:{
					int ns = 0;
					if (packet.getResponseState()){
						if (path[streamID] == 0){
							ns = send(localSocks[streamID], payload, packet.getPayloadSize(), 0);
							if(ns){
								printf("Enviou %d para browser resposta\n", ns);
							}else{
								printf("Nao enviou para browser resposta! [%d]\n", ns);
							}
							path[streamID] = invertVector(p.first);
							printf("Sucesso %d\n", path[streamID]);
							localSlaves.sendWork(streamID);
						}
					}else{
						closeLocal(streamID);
					}
					break;
				}
		        case 3:{//talk
					int ns = 0;
		            if (packet.isExitNode()){//exit sock
						puts("Exit");
		                ns = send(exitSocks[streamIdentifier[p]], payload, packet.getPayloadSize(), MSG_NOSIGNAL);
						if(ns){
							printf("Enviou %d para exit\n", ns);
						}else{
							printf("Nao enviou para exit! [%d]\n", ns);
						}
						// printf("Recebeu para exit %d\n", streamIdentifier[p]);
		                // cout << "Enviar Destino: "<< (char*)packet.getPayload()<<"\n";
		            }else{//localsock
						puts("Local");
						if(localSocks[streamID]!= -1){
							ns = send(localSocks[streamID], payload, packet.getPayloadSize(), MSG_NOSIGNAL);
							if(ns){
								printf("Enviou %d para local\n", ns);
							}else{
								printf("Nao enviou para local! [%d]\n", ns);
							}	
						}
						// printf("Recebeu para local %d\n", streamID);
						// cout << "Local: "<< (char*)packet.getPayload()<<"\n";
		            }
		            break;
				}
		        case 4://end
					puts("Recebeu fim de streamID");
		            if (packet.isExitNode())
		                closeExit(p);
		            else
		                closeLocal(streamID);
		            break;
		    }     
		} else {
			manager.buildPatchUp(direction, nodeID);
			close(neighbors[direction]);
			neighbors[direction] = -1;
			availableNeighbor[direction] = 0;
			manager.connectToManager();
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
		client = accept(server, (struct sockaddr*) &clientAddr, &addrLen);
		if (client > 0){
			if ((recv(client, buffer, MANAGERPACKET, 0)) > 0){
				if (buffer[0] < 4) {
					availableNeighbor[buffer[0]] = 1;
					neighbors[buffer[0]] = client;

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
		while (manager.read() > 0){
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
							availableNeighbor[d] = 1;
							send(neighbors[d], &dir, 1, 0);
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
	return NULL;
}



int main(int argc, char const *argv[]){
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
