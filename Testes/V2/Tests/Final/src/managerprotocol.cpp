#include "managerprotocol.h"

//	  1byte		 |          2bytes
// 	<Register>	 |	      <clientID>
//               |
//               |      1bytes(2/3/4/0)	      |		     1byte
// 	<Approval>	 |	  <Type/Not accepted>     |  <Nr Awaited Connections>
//               |                            |
//               |       1byte(0/1/2/3)       |          4bytes
//  <Conn new>   |        <direction>         |           <IP>    
//               |                            |
//               |                            |          2bytes
//  <Patch up>   |        <direction>         |        <clientID> 


namespace std {

    void ManagerProtocol::setManagerConf(unsigned int ip, int port){
        this->ip = ip;
        this->port= port;
    }
    
    int ManagerProtocol::connectToManager(){
        if (manager == -1){
            pthread_mutex_lock(&mutex);
            while ((manager = tcpClientSocket(ip, port)) < 0){
                sleep(1);
                //erro - Manager not available
            }
            pthread_cond_signal(&cond);
            pthread_mutex_unlock(&mutex);
            }
        return 0;
    }

    int ManagerProtocol::write(){
        if ((n = send(manager, packet, MANAGERPACKET, 0)) > 0)
           return n;
        close(manager);
        manager = -1;
        return -1;//Erro connection closed
    }

    int ManagerProtocol::read(){
        if ((n = recv(manager, packet, MANAGERPACKET, 0)) > 0)
            return n;
        
        close(manager);
        manager = -1;
        return -1;//Erro connection closed
    }

    void ManagerProtocol::disconnected(){
        if (n == 0){
            pthread_mutex_lock(&mutex);
            pthread_cond_wait(&cond, &mutex);
            pthread_mutex_unlock(&mutex);
        }
    }

    void ManagerProtocol::buildRegisterPacket(unsigned short id){
        memset(packet, 0 , MANAGERPACKET);
        packet[0] = 1;
        packet[1] = id;
        packet[2] = id>>8;
        //certificado e outras conices
    }

    void ManagerProtocol::buildPatchUp(unsigned char dir, unsigned char id){
        memset(packet, 0 , MANAGERPACKET);
        packet[0] = 4;
        packet[1] = dir;
        packet[2] = id;
        packet[3] = id>>8;
    }

    int ManagerProtocol::checkPacket(){
        if (packet[0] > 0 && packet[0] < 5)
            return packet[0];
        return -1;
    }
    int ManagerProtocol::getNodeType(){
        if (packet[1] == 0 || (packet[1] > 1 && packet[1] < 5)) 
            return packet[1];
        return -1;
    }

    int ManagerProtocol::getConnectingNeighbors(){
        if (packet[2] < 5) 
            return packet[2];
        return -1; // invalid awaited connections
    }

            
    int ManagerProtocol::getNeighborDir(){
        if (packet[1] < 4)
            return packet[1];
        return -1;
    }

    unsigned int ManagerProtocol::getNeighborIp(){
        return (packet[2] | (packet[3] << 8) | (packet[4] << 16) | (packet[5] << 24));
    }

}