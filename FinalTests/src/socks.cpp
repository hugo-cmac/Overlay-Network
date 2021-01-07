
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
                    bool res = false;
                    if(buffer[1] == 0x01){
                        res = true;
                        addr = &buffer[2];
                        payload = 4;
                    }
                    return res;
            }
        }
        return false;
    }
    
    bool Socks::handleConnectionReq(){ // Ler pacote Client connection request
        if (version == 0x05){
            nread = recv(fd, buffer, SOCKSIZE, 0);
            if(nread > 0){
                addr_type = buffer[3];
                if (addr_type == 0x01 || addr_type == 0x03){
                    addr = &buffer[4];
                    payload = nread - 3;
                    return true;
                }      
            }
        }else if(version == 0x04){
            addr_type = 0x01;
            return true;
        }
        //nao suportado/ erro
        return false;
    }

    int Socks::responsePacket(){
        if(version == 0x04){
            buffer[1] = 0x5A;
            return send(fd, buffer, nread, 0);;  
        }else{
            buffer[1] = 0x00;
            return send(fd, buffer, nread, 0);
        }
    }

    unsigned char* Socks::getAddr(){
        if(version == 0x04){
            unsigned char aux[6];
            aux[0] = addr[2];   aux[1] = addr[3];   aux[2] = addr[4];   aux[3] = addr[5];
            aux[4] = addr[0];   aux[5] = addr[1];
            memcpy(addr, aux, 6);
            return addr;
        } else if (version == 0x05){
            return addr;
        }
        return NULL;
    }

    int Socks::getSize(){
        return payload;
    }

    unsigned char Socks::getAddrType(){
        if (addr_type == 0x01) //ipv4
            return 0;
        return 1;
    }

}