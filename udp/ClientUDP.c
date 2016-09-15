
/*
** ClientUDP.c -- a datagram "client" demo
*/
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <limits.h>  // for INT_MAX
#define MAXBUFLEN 100
#define TENRADIX 10
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
} __attribute__((__packed__));

typedef struct request request_t;
struct response
{
  uint8_t TML;
  uint8_t requestID;
  uint8_t errorCode;
  int32Store result;
} __attribute__((__packed__));
typedef struct response response_t;
// get sockaddr, IPv4 or IPv6:


int getIntParam(int loc, char *argv[]){
    char *p;
    int num;
    int retInt = 0;
    long conver = strtol(argv[loc], &p, TENRADIX);
    if (errno != 0 || *p != '\0' || conver > INT_MAX)
        {
            perror("talker usage: talker serverName portNumber\n");
            exit(1);
        }
    else
        {
            retInt = conver;
        }
    return retInt;
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

request_t makeRequest(uint8_t tml, uint8_t requestID, uint8_t OPCode, uint8_t numOperands, int16_t opOne, int16_t opTwo){
    request_t retRequest;

    retRequest.TML = tml;
    retRequest.requestID = requestID;
    retRequest.opCode = OPCode;
    retRequest.numOperands = numOperands;

    int16Store operandOneStore;
    operandOneStore.theInt = opOne;
    getBytesFrom16(operandOneStore.theBytes, operandOneStore.theInt);
    retRequest.operandOne = operandOneStore;

    int16Store operandTwoStore;
    operandTwoStore.theInt = opTwo;
    getBytesFrom16(operandTwoStore.theBytes, operandTwoStore.theInt);
    retRequest.operandTwo = operandTwoStore;
    return retRequest;
}

request_t remakeRequest(unsigned char * theBytes){
    request_t retRequest;
    int16Store operandOneStore;
    int16Store operandTwoStore;
    retRequest.TML = theBytes[0];
    retRequest.requestID = theBytes[1];
    retRequest.opCode = theBytes[2];
    retRequest.numOperands = theBytes[3];

    operandOneStore.theBytes[0] = theBytes[4];
    operandOneStore.theBytes[1] = theBytes[5];
    operandOneStore.theInt = get16FromBytes(operandOneStore.theBytes);

    operandTwoStore.theBytes[0] = theBytes[6];
    operandTwoStore.theBytes[1] = theBytes[7];
    operandTwoStore.theInt = get16FromBytes(operandTwoStore.theBytes);

    retRequest.operandOne = operandOneStore;
    retRequest.operandTwo = operandTwoStore;
    return retRequest;
}

response_t remakeResponse(unsigned char * theBytes){
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

response_t makeResponse(uint8_t tml, uint8_t requestID, uint8_t errorCode, int32_t result){
    response_t returnResponse;
    returnResponse.TML = tml;
    returnResponse.requestID = requestID;
    returnResponse.errorCode = errorCode;
    int32Store resultToStore;
    resultToStore.theInt = result;
    getBytesFrom32(resultToStore.theBytes, resultToStore.theInt);

    returnResponse.result = resultToStore;
    return returnResponse;
}

int32Store calcResult(request_t toProcess){
    int32Store returnResult;
    toProcess.opCode;
    switch(toProcess.opCode){
    case ADDOP:
        returnResult.theInt = toProcess.operandOne.theInt + toProcess.operandTwo.theInt;
        break;
    case SUBOP:
        returnResult.theInt = toProcess.operandOne.theInt - toProcess.operandTwo.theInt;
        break;
    case OROP:
        returnResult.theInt = toProcess.operandOne.theInt | toProcess.operandTwo.theInt;
        break;
    case ANDOP:
        returnResult.theInt = toProcess.operandOne.theInt & toProcess.operandTwo.theInt;
        break;
    case RSOP:
        returnResult.theInt = toProcess.operandOne.theInt >> toProcess.operandTwo.theInt;
        break;
    case LSOP:
        returnResult.theInt = toProcess.operandOne.theInt << toProcess.operandTwo.theInt;
        break;
    default:
        break;
    }
    getBytesFrom32(returnResult.theBytes, returnResult.theInt);
    return returnResult;
}


response_t processRequest(unsigned char * targetRequest){
    response_t finalResponse;
    request_t currentRequest = remakeRequest(targetRequest);
    finalResponse.TML = 7;
    finalResponse.requestID = currentRequest.requestID;
    finalResponse.errorCode = 0;
    finalResponse.result = calcResult(currentRequest);
    return finalResponse;
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

int clientRun(int argc, char *argv[]){
    uint8_t keepGoing = 1;
    int16_t operandOne;
    int16_t operandTwo;
    uint8_t requestID = 0;
    uint8_t totMessLength = 8;
    uint8_t OpCode;
    uint8_t operandCount;
    uint8_t responseLength;

    int sockfd;
	int userInputs[5];
	unsigned char buf[MAXBUFLEN];
	unsigned char sendBytes[MAXSENDLEN];
	unsigned char recBytes[MAXRECLEN];
	int portNumber;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;

	if (argc != 3) {
		fprintf(stderr,"usage: talker serverName portNumber MessageBytes\n");
		exit(1);
	}
	portNumber = getIntParam(2, argv);
	memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_DGRAM;

    if ((rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
            return 1;
        }

	// loop through all the results and make a socket
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("talker: socket");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "talker: failed to create socket\n");
		return 2;
	}
	int z;
	while (keepGoing == 1){
            printf("Starting request # %d.\n", ++requestID);
            printf("Please enter the first operand: ");
            scanf("%" SCNd16, &operandOne);
            printf("You entered %d\n", operandOne);
            printf("Please enter the second operand: ");
            scanf("%" SCNd16, &operandTwo);
            printf("You entered %d\n", operandTwo);
            printf("Please enter the opCode of your desired operation.\n");
            printf("0 -> +\n");
            printf("1 -> -\n");
            printf("2 -> |\n");
            printf("3 -> &\n");
            printf("4 -> >>\n");
            printf("5 -> <<\n");
            printf("OP Code: ");
            scanf("%" SCNd8, &OpCode);
            operandCount = 2;

            request_t currentRequest = makeRequest(totMessLength, requestID, OpCode, operandCount, operandOne, operandTwo);
            getTotalMessage(sendBytes, currentRequest);

            int j;

            printf("The Bytes Client intends to send are: ");
            for(j = 0; j < 8; j++){
                printf(" %02x", sendBytes[j] & 0xff);
            }
            printf("\n");


            printf("Client: starting sendto...\n");
            if ((numbytes = sendto(sockfd, sendBytes, totMessLength, 0, p->ai_addr, p->ai_addrlen)) == -1) {
                perror("talker: sendto");
                exit(1);
            }
            printf("Client: waiting to recvfrom...\n");
            if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0, p->ai_addr, &p->ai_addrlen)) == -1) {
            perror("Client: recvfrom");
            exit(1);
            }
            responseLength = buf[0];
            int totBytes;
            for(totBytes = 0; totBytes < responseLength; totBytes++){
                recBytes[totBytes] = buf[totBytes];
            }

            printf("The Bytes Client got back are: ");
            for(j = 0; j < 7; j++){
                printf(" %02x", recBytes[j] & 0xff);
            }
            printf("\n");



            response_t currentResponse = remakeResponse(recBytes);
            printf("The server replied with the following result: %d\n", currentResponse.result.theInt);
            printf("Would you like to continue? (1 for yes, 0 for no): ");
            scanf("%d" SCNd8, &keepGoing);
	}
	freeaddrinfo(servinfo);
	close(sockfd);
	return 0;
}

int main(int argc, char *argv[])
{
    int zalgo = 0;
    unsigned char funnE[8];
    request_t myRequest = makeRequest(8, 7, 4, 2, 240, 4);
    getTotalMessage(funnE, myRequest);
    printf("The Bytes Found as the message to send were: ");
    int j;
    for(j = 0; j < 8; j++){
        printf(" %02x", funnE[j] & 0xff);
    }
    printf("\n");
    zalgo = clientRun(argc, argv);


    return zalgo;
}

