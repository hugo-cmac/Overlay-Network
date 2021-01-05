#include "clientprotocol.h"

byte availableNeighbor[4];
byte type;
int neighbors [5];

unsigned int timeR;

unsigned int random(int n){
    timeR++;
    srand(timeR);
    return rand() % n;
}

unsigned int invertVector(unsigned int vector){
    unsigned int inverse = 0;
    inverse |= (vector>>16 & 0x03);
    inverse |= (vector>>24 & 0x03)<<8;
    inverse |= (vector & 0x03)<<16;
    inverse |= (vector>>8  & 0x03)<<24;

    return inverse;
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

byte invertDirection(byte dir){
	switch(dir){
		case UP:
			return DOWN;
		case RIGHT:
			return LEFT;
		case DOWN:
			return UP;
		case LEFT:
			return RIGHT;
	}
	return 0;
}


namespace std{


    byte ClientProtocol::getRandomDirection(byte hop){
        int i = random(4);
        while(!availableNeighbor[i] || listenDirection == i){
            i = random(4);
        }
        byte dir = i;
        buffer[0] |= (dir & 0x03)<<(hop<<1);
            
        return dir;
    }

    byte ClientProtocol::getNewPath(byte vector[4]){
        int dir[3]={0};
        int f = 0, s = 0, t = 0;
                
        while(f < 4){
            if (vector[f]){
                vector[f] -= 1;
                dir[s++] = f;
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

    byte ClientProtocol::recalc(byte hop){
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
            
    ClientProtocol::ClientProtocol(short dir){
        if (dir != -1){
            buffer = (byte*) malloc(PACKET);
            this->listenDirection = dir;
        }
        this->direction = dir; 
    }

    int ClientProtocol::read(){
        memset(buffer, 0, PACKET);
        n = recv(neighbors[listenDirection], buffer, PACKET, 0);
        if (n < 0)
            return -1;
        return n;
    } 

    int ClientProtocol::write(){
        printBinary(buffer[0]);
        printBinary(buffer[1]);
        n = send(neighbors[direction], buffer, PACKET, 0);
        if (n < 0)
            return -1;
        return n;
    }

    int ClientProtocol::getNextDirection(){
        byte type = (buffer[1] >> 6) & 0x03;//saber se é new ou nao
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

    unsigned int ClientProtocol::getVector(){
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

    byte ClientProtocol::getStreamID(){	
	    return buffer[1] & 0x1f;
    }

    byte ClientProtocol::getAddrType(){
        return (buffer[1] & 0x20)>>5;
    }

    byte* ClientProtocol::getAddr(){
        return &buffer[2];
    }

    bool ClientProtocol::getResponseState(){
        if ((buffer[1] >> 5) & 0x01)
            return true;
        return false;
    }

    byte* ClientProtocol::getPayload(){		
        return &buffer[2];
    }

    void ClientProtocol::buildNew(bool domain, short streamID, byte* payload){
        memset(buffer, 0, PACKET);
        direction = getRandomDirection(3);
        buffer[0] |= 2 & 0x03;

        buffer[1] = NEW<<6;
        if (domain){
            buffer[1] |= 1<<5;
            memcpy(&buffer[2], payload, payload[0]+3);
        }else{
            memcpy(&buffer[2], payload, 6);
        }
        buffer[1] |= streamID & 0x1f;
    }

    void ClientProtocol::buildResponse(short streamID, unsigned int vector, bool success){
        memset(buffer, 0, PACKET);
        
        buffer[0] = getNewPath((byte*) & vector);
        buffer[1] = RESP<<6;

        if (success)
            buffer[1] |= 1<<5;
        buffer[1] |= streamID & 0x1f;
        getNextDirection();
    }

    void ClientProtocol::buildTalk(short streamID, unsigned int vector, bool exit, byte* payload){
       
        buffer = payload;
        byte circuit = getNewPath((byte*) &vector);

        buffer[0] = circuit;
        buffer[1] = TALK<<6;

        if (exit)
            buffer[1] |= 1<<5;
        buffer[1] |= streamID & 0x1f;
        getNextDirection();
    }

    void ClientProtocol::buildEnd(short streamID, unsigned int vector, bool exit){
        memset(buffer, 0, PACKET);
        
        byte circuit = getNewPath((byte*) &vector);
        
        buffer[0] = circuit;
        buffer[1] = END<<6;
        
        if (exit)
            buffer[1] |= 1<<5;
        buffer[1] |= streamID & 0x1f;
        getNextDirection();
    }

    bool ClientProtocol::isExitNode(){
        if ((buffer[1]>>5) & 0x01) 
            return true;
        return false;
    }
			

}