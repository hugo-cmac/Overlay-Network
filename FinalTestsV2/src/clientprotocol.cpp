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

void printHeader(byte *head){
    byte bit = (head[0]>>6) & 0x03;
    switch (bit){
        case UP:
            printf("UP ");
            break;
        case RIGHT:
            printf("RIGHT ");
            break;
        case DOWN:
            printf("DOWN ");
            break;
        case LEFT:
            printf("LEFT ");
            break;
        
        default:
            printf("[ ] ");
            break;
    }

    bit = (head[0]>>4) & 0x03;
    switch (bit){
        case UP:
            printf("UP ");
            break;
        case RIGHT:
            printf("RIGHT ");
            break;
        case DOWN:
            printf("DOWN ");
            break;
        case LEFT:
            printf("LEFT ");
            break;
        
        default:
            printf("[ ] ");
            break;
    }

    bit = (head[0]>>2) & 0x03;
    switch (bit){
        case UP:
            printf("UP ");
            break;
        case RIGHT:
            printf("RIGHT ");
            break;
        case DOWN:
            printf("DOWN ");
            break;
        case LEFT:
            printf("LEFT ");
            break;
        
        default:
            printf("[ ] ");
            break;
    }

    bit = head[0] & 0x03;
    printf("HOPS: %d\n", bit);

    bit = (head[1]>>6) & 0x03;
    switch (bit){
        case NEW:
            printf("NEW ");
            break;
        case RESP:
            printf("RESP ");
            break;
        case TALK:
            printf("TALK ");
            break;
        case END:
            printf("END ");
            break;
        
        default:
            printf("[ ] ");
            break;
    }

    bit = (head[1]>>5) & 0x01;

    if (bit){
        printf("EXIT ");
    }else{
        printf("LOCAL ");
    }
    
    bit = head[1] & 0x1f;
    printf("Stream ID: %d\n", bit);

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

byte invertCircuit(byte circuit){
    byte inverse = 0;
    inverse |= invertDirection(circuit>>2 & 0x03) << 6;
    inverse |= invertDirection(circuit>>4 & 0x03) << 4;
    inverse |= invertDirection(circuit>>6 & 0x03) << 2;
    
    // inverse |= (vector>>16 & 0x03);
    // inverse |= (vector>>24 & 0x03)<<8;
    // inverse |= (vector & 0x03)<<16;
    // inverse |= (vector>>8  & 0x03)<<24;

    return inverse;
}


namespace std{


    byte ClientProtocol::getRandomDirection(byte hop){
        int i = random(4);
        while(!availableNeighbor[i] || listenDirection == i){
            //printf("a");
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
            printf("b");
            if (vector[f]){
                vector[f] -= 1;
                dir[s++] = f;
            }else{
                f++;
            }
        }
            
        f = random(3);
        while(!availableNeighbor[dir[f]]){
            printf("c");
            f = random(3);
        }

        s = random(3);
        while(s == f){
            printf("i");
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
            
    ClientProtocol::ClientProtocol(int dir){
        if (dir != -1){
            buffer = (byte*) malloc(PACKET);
            this->listenDirection = dir;
        }
        this->direction = dir; 
    }

    int ClientProtocol::read(){
        memset(buffer, 0, PACKET);
        n = recv(neighbors[listenDirection], buffer, PACKET, MSG_WAITALL);
        if (n > 0){
            return n;
        }
        return -1;
    } 

    int ClientProtocol::write(){
        n = send(neighbors[direction], buffer, PACKET, 0);
        if (n > 0){
            return n;
        }
        return -1;
    }

    byte* ClientProtocol::getPacket(){
        return buffer;
    }

    void ClientProtocol::setPacket(byte* buff){
        this->buffer = buff;
    }

    byte ClientProtocol::getDirection(){
        return direction;
    }

    byte ClientProtocol::getListenDirection(){
        return listenDirection;
    }

    int ClientProtocol::getNextDirection(){
        byte type = (buffer[1] >> 6) & 0x03;//saber se é new ou nao
        byte hops = (buffer[0]) & 0x03;//numero de hops
            
        if (hops == 0){
            return ((int)type+1);
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

    byte ClientProtocol::getCircuit(){
        byte vector = buffer[0] & 0xfc;    
        // int temp=0;
                        
        // temp = (buffer[0]>>2) & 0x03;
        // vector += 1<<(temp<<3);

        // temp = (buffer[0]>>4) & 0x03;
        // vector += 1<<(temp<<3);
                        
        // temp = (buffer[0]>>6) & 0x03;
        // vector += 1<<(temp<<3);

        return vector;
    }

    int ClientProtocol::getStreamID(){	
	    return (buffer[1] & 0x1f) << 3 | (buffer[2] >> 5);
    }

    bool ClientProtocol::getResponseState(){
        if ((buffer[1] >> 5) & 0x01)
            return true;
        return false;
    }

    byte* ClientProtocol::getPayload(){		
        return &buffer[4];
    }

    int ClientProtocol::getPayloadSize(){
        return ((((buffer[2] & 0x1f) << 8)) | buffer[3]);
    }

    void ClientProtocol::buildNew(int streamID, byte* payload, int size){
        memset(buffer, 0, PACKET);
        direction = getRandomDirection(3);

        buffer[0] |= 2 & 0x03;
        buffer[1] = NEW<<6;   

        memcpy(&buffer[4], payload, size);  

        buffer[1] |= (streamID >> 3) & 0x1f;
        buffer[2] = ((streamID & 0x07) << 5) | ((size >> 8) & 0x1f);
        buffer[3] = size;
    }

    void ClientProtocol::buildResponse(int streamID, byte circuit, bool success){        
        buffer[0] = circuit | 0x03;//getNewPath((byte*) &vector);
        buffer[1] = RESP<<6;

        if (success){
            buffer[1] |= 1<<5;
        }else{
            buffer[1] |= 0<<5;
        }

        buffer[1] |= (streamID >> 3) & 0x1f;

        getNextDirection();
    }

    void ClientProtocol::buildTalk(int streamID, byte circuit, bool exit, byte* payload, int size){
        buffer = payload;

        buffer[0] = circuit | 0x03;
        buffer[1] = TALK<<6;

        if (exit)
            buffer[1] |= 1<<5;

        buffer[1] |= (streamID >> 3) & 0x1f;
        buffer[2] = ((streamID & 0x07) << 5) | ((size >> 8) & 0x1f);
        buffer[3] = size;

        getNextDirection();
    }

    void ClientProtocol::buildEnd(int streamID, byte circuit, bool exit){
        memset(buffer, 0, PACKET);

        buffer[0] = circuit | 0x03;
        buffer[1] = END<<6;
        
        if (exit)
            buffer[1] |= 1<<5;
        
        buffer[1] |= (streamID >> 3) & 0x1f;
        buffer[2] = (streamID & 0x07) << 5;

        getNextDirection();
    }

    bool ClientProtocol::isExitNode(){
        if ((buffer[1] >> 5) & 0x01) 
            return true;
        return false;
    }

}