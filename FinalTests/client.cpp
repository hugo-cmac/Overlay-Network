#include <cstdio>
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


int webConnect(byte type, byte* addr){
	uint16_t port = 0;
	int sock = -1;
	if (type){ // dominio
		int size = addr[0];

		char domain[size+1] = {0};
		memcpy(domain, &addr[1], size);

		char printbuffer[256];
		sprintf(printbuffer,"Domain: %s", domain);
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
    int n = 1;

    byte* buffer = (byte*)malloc(PACKET);
    ClientProtocol packet(-1);
    pair<unsigned int, short> p = pathIdentifier[streamID];
	printf("Exit inicializado %d\n", streamID);
    while (n){
    	memset(buffer, 0, PACKET);
		n = recv(exitSocks[streamID], &buffer[2], PAYLOAD, 0);
		if (n > 0) {
			packet.buildTalk(p.second, p.first, false, buffer);
			packet.write();
		} else {
			return;
		}
    }
}

void proxyLocalHandler(ClientProtocol packet, int streamID){
	byte* buffer = (byte*)malloc(PACKET);
	int n = 1;
    printf("Local inicializado %d\n", streamID);
	while(n){
		memset(buffer, 0, PACKET);
		n = recv(localSocks[streamID], &buffer[2], PACKET, 0);
		if (n > 0){
			packet.buildTalk(streamID, path[streamID], true, buffer);
			packet.write();
		}else{
			return;
		}
	}
}

void proxyServerProcedure(int proxyClient){

    Socks client(proxyClient);
	ClientProtocol packet(0);

    if (client.handleGreetingPacket()) {
		
    	if (client.handleConnectionReq()) { // se for dominio ou ipv4 retorna true
    		byte* addr = client.getAddr();
    		byte type = client.getAddrType();

			short streamID = findEmpty(localSocks, proxyClient);
			if (streamID != -1){
	    		packet.buildNew(type, streamID, addr);
				packet.write();

				while (!path[streamID]){
					puts("preso");
					sleep(1);
					if (localSocks[streamID] == -1){
						return;
					}
				}
				puts("ligacao bem estabelecida");
				if (client.responsePacket() > 0) 
					proxyLocalHandler(packet, streamID);
                packet.buildEnd(streamID, path[streamID], true);
                packet.write();
				closeLocal(streamID);
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
			setsockopt(server, SOL_TCP, TCP_NODELAY, &one, sizeof(one));
        	setsockopt(client, SOL_TCP, TCP_NODELAY, &one, sizeof(one));
            localSlaves.sendWork(client);
        }
    }
    pthread_exit(NULL);
}

void forwardingHandler(int direction){
    int n = 1, streamID, result;
    ClientProtocol packet(direction);

    while (n){
    	if (packet.read() > 0){
			streamID = 0;
		    result = packet.getNextDirection();//caso seja para encaminhar faz aqui a alteracao do salto
		    streamID = packet.getStreamID();
		    pair<unsigned int,short> p(packet.getVector(), streamID);

	        switch(result){
		       	case 0://é para encaminhar  
				   	printf("Encaminhou da %d\n", direction);
		            packet.write();
		            break; 
		        case 1:{//new exitSocket
					pair<unsigned int,short> r(invertVector(p.first), streamID);

		            int webSocket = webConnect(packet.getAddrType(), packet.getAddr());
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
				case 2:
					if (packet.getResponseState()){
						
						if (path[streamID] == 0){
							
							path[streamID] = invertVector(p.first);
							printf("Sucesso %d\n", path[streamID]);
						}
		                    
					}else{
						closeLocal(streamID);
					}
					break;
		        case 3://talk
		            if (packet.isExitNode()){//exit sock
		                send(exitSocks[streamIdentifier[p]], packet.getPayload(), PACKET, 0);
						printf("Recebeu para exit %d\n", streamIdentifier[p]);
		                //cout << "Exit: "<< (char*)packet.getPayload()<<"\n";
		            }else{//localsock
		            	//cout << "Local: "<< (char*)packet.getPayload()<<"\n";
		                send(localSocks[streamID], packet.getPayload(), PACKET, 0);
						printf("Recebeu para local %d\n", streamID);
		            }
		            break;
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
    localSlaves.init(MAXFDS, (void*) &localSlaves, proxyServerProcedure);
    exitSlaves.init(MAXFDS, (void*) &exitSlaves, exitNodeHandler);
    
    nodeID = atoi(argv[1]);
	unsigned int managerAddr = 0;

    inet_pton(AF_INET, "172.20.10.4", &managerAddr);
	manager.setManagerConf(managerAddr, 7777);
	pthread_create(&managerSlave, NULL, Manager, NULL);
	pthread_detach(managerSlave);

    proxyServerHandler();

    return 0;
}
