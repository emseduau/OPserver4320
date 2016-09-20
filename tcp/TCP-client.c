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
#include <time.h>

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

typedef struct int16Store{ unsigned char theBytes[3]; int16_t theInt; } int16Store; //contains the 16bit- int and byte representation of the int
typedef struct int32Store{ unsigned char theBytes[5]; int32_t theInt; } int32Store;;//contains the 32bit- int and byte representation of the int



struct request //Struct containing each part of a server request
{
  uint8_t TML;		//Total Message Length
  uint8_t requestID;	//ID of the request (request number)
  uint8_t opCode;	//Code for the operation to perform
  uint8_t numOperands;	//Number of operands for operation
  int16Store operandOne;
  int16Store operandTwo;
} __attribute__((__packed__));//minimizes space

typedef struct request request_t; //struct for request to server


struct response //Struct containing each part of an incoming response from the server
{
  uint8_t TML;		//Total Message Length
  uint8_t requestID;	//ID of the request (request number)
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



int32_t get32FromBytes(unsigned char * theBytes){ //get int32 from byte array
   int32_t retInt = 0;
   retInt = (retInt & 0x00ffffff ) | (theBytes[0] << 24);
   retInt = (retInt & 0xff00ffff ) | (theBytes[1] << 16);
   retInt = (retInt & 0xffff00ff ) | (theBytes[2] << 8);
   retInt = (retInt & 0xffffff00 ) | (theBytes[3]);
   return retInt;
}


void getBytesFrom16(unsigned char * theBytes, int16_t unpackInt){ // get byte array from a int16
   theBytes[0] = (unpackInt >> 8 )  & 0x00ff;
   theBytes[1] = (unpackInt)        & 0x00ff;
   theBytes[2] = '\0';
}


void getTotalMessage(unsigned char * theBytes, request_t targetRequest){ //Converts request_t struct into array of bytes to send
    theBytes[0] = targetRequest.TML;
    theBytes[1] = targetRequest.requestID;
    theBytes[2] = targetRequest.opCode;
    theBytes[3] = targetRequest.numOperands;
    theBytes[4] = targetRequest.operandOne.theBytes[0];
    theBytes[5] = targetRequest.operandOne.theBytes[1];
    theBytes[6] = targetRequest.operandTwo.theBytes[0];
    theBytes[7] = targetRequest.operandTwo.theBytes[1];
}

response_t remakeResponse(unsigned char * theBytes){ //Fills in response_t struct with data received from server
    response_t returnResponse;
    returnResponse.TML = theBytes[0];
    returnResponse.requestID = theBytes[1];
    returnResponse.errorCode = theBytes[2];
    int32Store resultToStore;

    resultToStore.theBytes[0] = theBytes[3];
    resultToStore.theBytes[1] = theBytes[4];
    resultToStore.theBytes[2] = theBytes[5];
    resultToStore.theBytes[3] = theBytes[6];
    resultToStore.theInt = get32FromBytes(resultToStore.theBytes);

    returnResponse.result = resultToStore;
    return returnResponse;
}

request_t getRequest(uint8_t reqNum) { //Prompts user for request info
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
}


long timediff(clock_t t1, clock_t t2) { //calculates elapsed time in milliseconds of clocks in main()
    long elapsed;
    elapsed = ((double)t2 - t1) / CLOCKS_PER_SEC * 1000;
    return elapsed;
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
  
  ////////////////////////////////////////////////
  clock_t t1, t2;  ///////////////////////////////
  int i;	   ///////////// Clock ///////////
  float x = 2.7182;/////////// Variables /////////
  long elapsed;	   ///////////////////////////////
  ////////////////////////////////////////////////
 
	if (argc != 3) {
	    fprintf(stderr,"usage: syntax: client <Host Name> <Port Number>\n");
	    exit(1);
	}
   
   
	memset(&hints, 0, sizeof hints);   //make sure hints is empty
	hints.ai_family = AF_UNSPEC; 	   //IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM;   //TCP socket

	if ((rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0) { //fills hints with address info
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
      
	  if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) { //connect to server
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
   int cont = 1;
   while(cont == 1) { // Loops until user selects not to after a request
      uint8_t reqNum = iNum;
      req = getRequest(reqNum);
      
      getTotalMessage(sendBytes, req); //Builds byte array to send to server
      
      
      printf("The Bytes Client intends to send are: ");
      int j;
      for(j = 0; j < 8; j++){
	printf(" %02x", sendBytes[j] & 0xff); 	//Print out bytes client is going to send
      }
      printf("\n");

      send(sockfd,sendBytes,8,0); //Sends the request to the server


/////////////////////////////////////////////////////////////////////
      t1 = clock();		       //////////////////////////////	
      for (i=0; i < 1000000; i++) {    //////Start Clock Timer///////
	x = x * 3.1415;		       //////////////////////////////
      }				       //////////////////////////////
/////////////////////////////////////////////////////////////////////

      
      
      if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) { //receive data from server
	perror("client: recv");
	exit(1);
      }

///////////////////////////////////////////////////////////////////////
	t2 = clock();		      ///////////////Stop//////////////
	elapsed = timediff(t1, t2);   ////////////Clock Timer//////////
///////////////////////////////////////////////////////////////////////
   
	int messageLength = buf[0]; //gets the length of received message from first byte
	
	int totBytes;
	for(totBytes = 0; totBytes < messageLength; totBytes++){ // Copy the received bytes to recBytes[]
	  recBytes[totBytes] = buf[totBytes];
	}

	
	int k;
	printf("\nThe Bytes Client received are: ");
	for(k = 0; k < 7; k++){
          printf(" %02x", recBytes[k] & 0xff); // Outputs the bytes received in hex form
	}
	printf("\n");
	
        
	response_t currentResponse = remakeResponse(recBytes); //fills a response_t struct with data that was received
	printf("The server replied with the following requestID: %d\n", currentResponse.requestID); //get requestID from response_t 
	printf("The server replied with the following result: %d\n", currentResponse.result.theInt); //get result from response_t currentResponse
	
	printf("Time elapsed: %ld ms\n", elapsed);
      
	printf("Would you like to continue? (1 for yes, 0 for no): ");
	scanf("%d" SCNd8, &cont);
        
	iNum = iNum + 1; //Increments the request number to give next request correct requestID
   }
   close(sockfd);
   
   return 0;
}

