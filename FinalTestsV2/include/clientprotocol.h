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
//   6bit    2bits | 2bits     1bits(0/1)      5bits    |   3bits         13bits      |    Resto
// <circuit> <hop> | <new>        NULL       <streamID> | <streamID>  <PayloadLength> |  <Payload>
// <circuit> <hop> | <resp>     <Sucess>     <streamID> | <streamID>  <PayloadLength> |  <Payload>
// <circuit> <hop> | <talk>   <local/exit>   <streamID> | <streamID>  <PayloadLength> |  <Payload>
// <circuit> <hop> | <end>    <local/exit>   <streamID> |   

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

byte invertDirection(byte dir);

byte invertCircuit(byte circuit);

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
            ClientProtocol(int dir);

            byte getNewPath(byte vector[4]);

            int read();

            int write();
            
            byte* getPacket();

            void setPacket(byte* buff);

            byte getDirection();

            byte getListenDirection();

            int getNextDirection();

            byte getCircuit();

            int getStreamID();

            bool getResponseState();

            byte* getPayload();

            int getPayloadSize();

            void buildNew(int streamID, byte* payload, int size);

            void buildResponse(int streamID, byte circuit, bool success);

            void buildTalk(int streamID, byte circuit, bool exit, byte* payload, int size);

            void buildEnd(int streamID, byte circuit, bool exit);
			
            bool isExitNode();

    };

}

#endif