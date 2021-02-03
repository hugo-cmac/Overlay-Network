#include "socks.h"

namespace std{
              
    void Socks::greetingsAnswer(bool auth){
        unsigned char answer[2] = {0x05, 0xff};
        if(auth){ // method accepted
            answer[1] = 0x00;
        }
        send(fd, answer, 2, 0);
    }

    Socks::Socks(int fd){
        this->fd = fd;
    }

    bool Socks::handleGreetingPacket(){ //return false = erro / true = autenticacao e suportada
        nread = recv(fd, buffer, SOCKSIZE, 0);
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
                    if(buffer[1] == 0x01){
                        return true;
                    }
                    return false;
            }
        }
        return false;
    }
    
    bool Socks::handleConnectionReq(){ // Ler pacote Client connection request
        if (version == 0x05){
            nread = recv(fd, buffer, SOCKSIZE, 0);
            if(nread > 0){
                return true;
            }
        }else if(version == 0x04){
            return true;
        }
        return false;
    }

    unsigned char* Socks::getPayload(){
        return buffer;
    }

    int Socks::getSize(){
        return nread;
    }
}