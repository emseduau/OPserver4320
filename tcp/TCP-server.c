/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <inttypes.h>

//#define PORT "10025"  // the port users will be connecting to
#define MAXDATASIZE 100
#define BACKLOG 10	 // how many pending connections queue will hold
#define MAXBUFLEN 100
#define TENRADIX 10
#define MAXSENDLEN 7
#define MAXRECLEN 8
#define ADDOP 0
#define SUBOP 1
#define OROP 2
#define ANDOP 3
#define RSOP 4
#define LSOP 5
typedef struct int16Store{ unsigned char theBytes[3]; int16_t theInt; } int16Store;
typedef struct int32Store{ unsigned char theBytes[5]; int32_t theInt; } int32Store;;
typedef enum { false, true } bool;
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

void sigchld_handler(int s)
{
	// waitpid() might overwrite errno, so we save and restore it:
	int saved_errno = errno;

	while(waitpid(-1, NULL, WNOHANG) > 0);

	errno = saved_errno;
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
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

void unloadResponse(unsigned char * theBytes, response_t targetResponse){
    theBytes[0] = targetResponse.TML;
    theBytes[1] = targetResponse.requestID;
    theBytes[2] = targetResponse.errorCode;
    theBytes[3] = targetResponse.result.theBytes[0];
    theBytes[4] = targetResponse.result.theBytes[1];
    theBytes[5] = targetResponse.result.theBytes[2];
    theBytes[6] = targetResponse.result.theBytes[3];
}

int main(int argc, char *argv[])
{
	int sockfd, new_fd, numbytes;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int rv;
   
   unsigned char buf[MAXBUFLEN];
	unsigned char sendBytes[MAXSENDLEN];
	unsigned char recBytes[MAXRECLEN];
   
   if (argc != 2) {
	    fprintf(stderr,"usage: syntax: server <Port Number>\n");
	    exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}
   
	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	printf("server: waiting for connections...\n");
	while(1) {  // main accept() loop
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}
      
		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);
		printf("server: got connection from %s\n", s);
      while(1){
      if ((numbytes = recv(new_fd, buf, MAXDATASIZE-1,0)) == -1) {
         perror("server: recv");
         exit(1);
      }
      int messageLength = buf[0];
      int totBytes;
      for(totBytes = 0; totBytes < messageLength; totBytes++){
            recBytes[totBytes] = buf[totBytes];
      }
      int j;
        printf("\nThe Bytes Server got are: ");
        for(j = 0; j < 8; j++){
            printf(" %02x", recBytes[j] & 0xff);
        }
        printf("\n");
        
        
        response_t currentResponse = processRequest(recBytes);
        unloadResponse(sendBytes, currentResponse);
        //printf("listener: packet is %d bytes long\n", numbytes);        
        
        
        printf("The Bytes Server intends to send are: ");
        for(j = 0; j < 7; j++){
            printf(" %02x", sendBytes[j] & 0xff);
        }
        printf("\n");  
        
        if (send(new_fd, sendBytes, 7, 0) == -1)
            perror("send");
            
      
		if (!fork()) { // this is the child process
			close(sockfd); // child doesn't need the listener
// 			if (send(new_fd, buf, 8, 0) == -1)
// 				perror("send");
			close(new_fd);
			exit(0);
		}
      }
		close(new_fd);  // parent doesn't need this
   }

	return 0;
}

