#ifndef SOCKS_H_
#define SOCKS_H_

#include <cstddef>
#include <cstring>

#include <sys/socket.h>

#define SOCKSIZE 300

namespace std{

    class Socks{

        private:
            unsigned char version = 0;

            int fd;
            int nread = -1;

            unsigned char buffer[SOCKSIZE] = {0};
             
            void greetingsAnswer(bool auth);
    
        public:
            Socks(int fd);
            
            bool handleConnectionReq();
            
            bool handleGreetingPacket();

            int responsePacket();

            unsigned char* getPayload();

            int getSize();
    };

}

#endif