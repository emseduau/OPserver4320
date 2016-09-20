/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <inttypes.h>
#include <arpa/inet.h>

//#define PORT "10025" // the port client will be connecting to 

#define MAXDATASIZE 100 // max number of bytes we can get at once 
#define MAXSENDLEN 8
#define MAXRECLEN 7
#define ADDOP 0
#define SUBOP 1
#define OROP 2
#define ANDOP 3
#define RSOP 4
#define LSOP 5

typedef struct int16Store{ unsigned char theBytes[3]; int16_t theInt; } int16Store;
typedef struct int32Store{ unsigned char theBytes[5]; int32_t theInt; } int32Store;;



struct request
{
  uint8_t TML;
  uint8_t requestID;
  uint8_t opCode;
  uint8_t numOperands;
  int16Store operandOne;
  int16Store operandTwo;
} __attribute__((__packed__));//minimizes space

typedef struct request request_t; //struct for request to server


struct response
{
  uint8_t TML;
  uint8_t requestID;
  uint8_t errorCode;
  int32Store result;
} __attribute__((__packed__));//minimizes space

typedef struct response response_t; //struct for response from server





// get sockaddr, IPv4 or IPv6:

void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}



int32_t get32FromBytes(unsigned char * theBytes){
   int32_t retInt = 0;
   retInt = (retInt & 0x00ffffff ) | (theBytes[0] << 24);
   retInt = (retInt & 0xff00ffff ) | (theBytes[1] << 16);
   retInt = (retInt & 0xffff00ff ) | (theBytes[2] << 8);
   retInt = (retInt & 0xffffff00 ) | (theBytes[3]);
   return retInt;
}

int16_t get16FromBytes(unsigned char * theBytes){
   int16_t retInt = 0;
   retInt = (retInt & 0x00ff ) | (theBytes[0] << 8);
   retInt = (retInt & 0xff00 ) | (theBytes[1]);
   return retInt;
}

void getBytesFrom32(unsigned char * theBytes, int32_t unpackInt){
   theBytes[0] = (unpackInt >> 24 );
   theBytes[1] = (unpackInt >> 16 ) & 0x000000ff;
   theBytes[2] = (unpackInt >> 8 )  & 0x000000ff;
   theBytes[3] = (unpackInt)        & 0x000000ff;
   theBytes[4] = '\0';
}

void getBytesFrom16(unsigned char * theBytes, int16_t unpackInt){
   theBytes[0] = (unpackInt >> 8 )  & 0x00ff;
   theBytes[1] = (unpackInt)        & 0x00ff;
   theBytes[2] = '\0';
}


void getTotalMessage(unsigned char * theBytes, request_t targetRequest){
    theBytes[0] = targetRequest.TML;
    theBytes[1] = targetRequest.requestID;
    theBytes[2] = targetRequest.opCode;
    theBytes[3] = targetRequest.numOperands;
    theBytes[4] = targetRequest.operandOne.theBytes[0];
    theBytes[5] = targetRequest.operandOne.theBytes[1];
    theBytes[6] = targetRequest.operandTwo.theBytes[0];
    theBytes[7] = targetRequest.operandTwo.theBytes[1];
}


request_t getRequest(uint8_t reqNum) {
   uint16_t op1;
   uint16_t op2;
   uint8_t opc;
   request_t cReq;
   
   printf("\n[0] - Addition(+)\n");
   printf("[1] - Subtraction(-)\n");
   printf("[2] - Logical OR(|)\n");
   printf("[3] - Logical AND(&)\n");
   printf("[4] - Shift Right(>>)\n");
   printf("[5] - Shift Left(<<)\n");
   printf("Enter Operation to be performed: ");
   scanf("%" SCNd8, &opc);
   
   printf("Please enter the first operand: ");
   scanf("%" SCNd16, &op1);
   printf("Please enter the second operand: ");
   scanf("%" SCNd16, &op2);
   
   cReq.TML = MAXSENDLEN;
   cReq.requestID = reqNum;
   cReq.opCode = opc;
   if(opc > 3) {
      cReq.numOperands = 1;
   }
   else {
      cReq.numOperands = 2;
   }
   
   int16Store op1Store;
   op1Store.theInt = op1;
   getBytesFrom16(op1Store.theBytes, op1Store.theInt);
   cReq.operandOne = op1Store;
   
   int16Store op2Store;
   op2Store.theInt = op2;
   getBytesFrom16(op2Store.theBytes, op2Store.theInt);
   cReq.operandTwo = op2Store;
   
   return cReq;
   
//    switch(input) {
//    case ADDOP:
// 
//       break;
//    case SUBOP:
//
//       break;
//    case OROP:
//
//       break;
//    case ANDOP:
//
//       break;
//    case RSOP:
//
//       break;
//    case LSOP:       
//
//       break;
//    default:
//       break;
//   }

}

int main(int argc, char *argv[])
{
	int sockfd, numbytes;  
	unsigned char buf[MAXDATASIZE];
   unsigned char sendBytes[MAXSENDLEN];
   unsigned char recBytes[MAXRECLEN];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];
   int portNum;
   request_t req;

 
	if (argc != 3) {
	    fprintf(stderr,"usage: syntax: client <Host Name> <Port Number>\n");
	    exit(1);
	}
   
   
	memset(&hints, 0, sizeof hints); //make sure hints is empty
	hints.ai_family = AF_UNSPEC; //IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM; //TCP socket

	if ((rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}
      
		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			perror("client: connect");
        close(sockfd);
			continue;
		}
      
      
		break;
	}
   

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}


	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure
   
   int iNum = 0;
   
   while(1) {
      uint8_t reqNum = iNum;
      req = getRequest(reqNum);
      
      getTotalMessage(sendBytes, req);
      
      
       printf("The Bytes Client intends to send are: ");
       int j;
       for(j = 0; j < 8; j++){
         printf(" %02x", sendBytes[j] & 0xff);
       }
       printf("\n");

      send(sockfd,sendBytes,8,0);
   
   	if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
   	    perror("client: recv");
   	    exit(1);
   	}
   
   	buf[numbytes] = '\0';
   
   	printf("client: received '%s'\n",buf);
      iNum = iNum + 1;
   }
	close(sockfd);
   
	return 0;
}

