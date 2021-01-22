#ifndef CLIENTPROTOCOL_H_
#define CLIENTPROTOCOL_H_

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <cstdio>

#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>

//Protocol
//   6bit    2bits | 2bits     1bits(0/1)      5bits    |   Rest
// <circuit> <hop> | <new>    <ip/dominio>   <streamID> | <Payload>
// <circuit> <hop> | <resp>     <Sucess>     <streamID> | <Payload>

//                   2bits      1bit(0/1)      5bits    |   Resto
// <circuit> <hop> | <talk>   <local/exit>   <streamID> | <Payload>
// <circuit> <hop> | <end>    <local/exit>   <streamID> | errr - qual foi

typedef unsigned char byte;

#define PACKET  1024
#define PAYLOAD 1020

enum DIRECTION {
    UP = 0,
    RIGHT,
    DOWN,
    LEFT
};

enum CONTENTTYPE{
    NEW = 0,
	RESP,
    TALK, 
    END 
};

//////////////////////////////////////////////////////////////////////////////////////////
//																						//
extern byte availableNeighbor[4];	//  // ////// // ////// //  // /////  ////// /////	//
extern byte type;					/// // //     // //  	//  // //  // //  // //  //	//
extern int neighbors [5]; 			////// /////  // // ///	////// /////  //  // /////	//
//									// /// //     // //  //	//  // //  // //  // // //	//
//									//  // ////// // //////	//  // /////  ////// //  //	//
//																						//
//////////////////////////////////////////////////////////////////////////////////////////

extern unsigned int timeR;

unsigned int random(int n);

unsigned int invertVector(unsigned int vector);

byte invertDirection(byte dir);

namespace std{

    class ClientProtocol{

        private:
            byte *buffer = NULL;
            int n = 0;

            byte listenDirection = 6;
            byte direction = -1;

            byte getRandomDirection(byte hop);

            

            byte recalc(byte hop);

        public:
            ClientProtocol(byte dir);

            byte getNewPath(byte vector[4]);

            int read();

            int write();

            int getNextDirection();

            unsigned int getVector();

            short getStreamID();

            bool getResponseState();

            byte* getPayload();

            int getPayloadSize();

            void buildNew(bool domain, short streamID, byte* payload, ushort size);

            void buildResponse(short streamID, unsigned int vector, bool success);

            void buildTalk(short streamID, byte circuit, bool exit, byte* payload, ushort size);

            void buildEnd(short streamID, byte circuit, bool exit);
			
            bool isExitNode();

    };

}

#endif