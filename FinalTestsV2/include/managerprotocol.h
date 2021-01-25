#ifndef MANAGERPROTOCOL_H_
#define MANAGERPROTOCOL_H_

#include <cstring>

#include <pthread.h>

#include "tcp.h"

//	  1byte		 |          2bytes
// 	<Register>	 |	      <clientID>
//               |
//               |      1bytes(2/3/4/0)	      |		     1byte
// 	<Approval>	 |	  <Type/Not accepted>     |  <Nr Awaited Connections>
//               |                            |
//               |       1byte(0/1/2/3)       |          4bytes
// <Connect new> |        <direction>         |           <IP>    
//               |                            |
//               |                            |          2bytes
//  <Patch up>   |        <direction>         |        <clientID> 

#define MANAGERPACKET 128

namespace std {

    class ManagerProtocol{
        private:
            unsigned int ip;
            int port = -1;

            int dir;

            pthread_cond_t cond = PTHREAD_COND_INITIALIZER;	
            pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

            int manager = -1;
            unsigned char packet[MANAGERPACKET] = {0};
            int n = 0;

        public:        
            void setManagerConf(unsigned int ip, int port);

            int connectToManager();

            int write();

            int read();

            void disconnected();

            void buildRegisterPacket(unsigned short id);

            void buildPatchUp(unsigned char dir, unsigned char id);

            int checkPacket();

            int getNodeType();

            int getConnectingNeighbors();
            
            int getNeighborDir();

            unsigned int getNeighborIp();
    };

}

#endif