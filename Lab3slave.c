
#include <pthread.h>
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

typedef struct int16store
{
  unsigned char theBytes[3];
  int16_t theInt;
} int16Store;

typedef struct int32Store
{
  unsigned char theBytes[5];
  int32_t theInt;
} int32Store;

uint8_t groupID = 15;
uint16_t magicNum = 0x1234;
struct ring theRing;

struct ring
{
  uint8_t mastGID;
  int16Store magic;
  uint8_t rID;
  unsigned char nextSlave[4];
  
}__attribute__((__packed__));

int16_t get16FromBytes(unsigned char * theBytes)
{
  int16_t retInt = 0;
  retInt = (retInt & 0x00ff ) | (theBytes[0] << 8);
  retInt = (retInt & 0x00ff ) | (theBytes[1]);
  return retInt;
}


void makeRing(struct ring *ringInfo, unsigned char * response)
{
  int16Store magicN;
  ringInfo->mastGID = response[0];
  magicN.theBytes[0] = response[1];
  magicN.theBytes[1] = response[2];
  magicN.theInt = get16FromBytes(magicN.theBytes);
  ringInfo->magic = magicN;
  ringInfo->rID = response[3];
  ringInfo->nextSlave[0] = response[4];
  ringInfo->nextSlave[1] = response[5];
  ringInfo->nextSlave[2] = response[6];
  ringInfo->nextSlave[3] = response[7];
}

void getPort(char *port, int portNum)
{
  char temp[6];
  sprintf(temp, "%d", portNum);

  int i;
  for(i = 0; i < 5; i++)
    {
      port[i] = temp[i];
    }
}

void setHints(struct addrinfo *temp)
{
  memset(temp, 0, sizeof temp); // make sure the struct is empty
  temp->ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
  temp->ai_socktype = SOCK_STREAM; // TCP stream sockets
  temp->ai_flags = AI_PASSIVE;     // fill in my IP for me
}

void setDgramHints(struct addrinfo *temp)
{
  memset(temp, 0, sizeof temp); // make sure the struct is empty
  temp->ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
  temp->ai_socktype = SOCK_DGRAM; // TCP stream sockets
  temp->ai_flags = AI_PASSIVE;     // fill in my IP for me
}

void makeRequest(unsigned char * theBytes[]){
  theBytes[0] = groupID;
  theBytes[1] = 0x12; // 18 in decimal.
  theBytes[2] = 0x34; // 52 in decimal.
}


unsigned char checksum(unsigned char message[], int nBytes) 
{
    unsigned char sum = 0;

    while (nBytes-- > 0)
    {
        int carry = (sum + *message > 255) ? 1 : 0;
        sum += *(message++) + carry;
    }

    return (~sum);
}



int main(int argc, char *argv[])
{
  int sockfd, result, bytes_sent;
  struct addrinfo hints;
  struct addrinfo *res;
  unsigned char req[3];
  unsigned char response[8];
  char s[INET6_ADDRSTRLEN];
  
  
  setHints(&hints);
  result = getaddrinfo(argv[1], argv[2], &hints, &res);
  
  
  sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  connect(sockfd, res->ai_addr, res->ai_addrlen);

  int check = 1;
  
  while(check == 1) {
    
    makeRequest(req);
    send(sockfd, req, 3, 0);
    recv(sockfd, response, 8, 0);
    check = 0;
    makeRing(&theRing, response);
    printf("\nGroup ID of Master: %d", theRing.mastGID);
    printf("\nRing ID is: %d", theRing.rID );
    printf("\nIP of next slave: %d.%d.%d.%d\n ", theRing.nextSlave[0], theRing.nextSlave[1], theRing.nextSlave[2], theRing.nextSlave[3]);
    check = 0;
  }
    close(sockfd);


    //Listen on port 10010 (MastGID*5) + RingID
    
    int sockUDP, new_fd, rslt, numbytes;
    socklen_t addr_size;
    struct addrinfo udphints, *udpres;
    struct sockaddr_storage their_addr;
    unsigned char buf[75];

    int portNum = 10010 + (theRing.mastGID * 5) + theRing.rID;
    char port[5];

    getPort(port, portNum);
    printf("\nPort to bind: %s\n", port);
    
    setDgramHints(&udphints);

    rslt = getaddrinfo(NULL,port, &udphints, &udpres);

    sockUDP = socket(res->ai_family,res->ai_socktype,res->ai_protocol);

    bind(sockUDP, res->ai_addr, res->ai_addrlen);

    listen(sockUDP,10);

    addr_size = sizeof their_addr;

    while(1) {

      new_fd = accept(sockUDP,(struct sockaddr *) &their_addr, &addr_size);

      if (new_fd == -1) {
	perror("accept");
	continue;
      }

      while(1)
	{
	 if ((numbytes = recv(new_fd, buf, 74,0)) == -1) {
         perror("server: recv");
         exit(1);
	}
	 unsigned char recMsg[numbytes];
	 int p;
	 for(p = 0; p < numbytes; p++)
	   {
	     recMsg[p] = buf[p];
	   }

	 //CHECK CHECKSUM OF RECEIVED MESSAGE
	 if(checksum(recMsg, numbytes) == 0)
	   {
	     //CHECK IF RING ID IS THIS SLAVE
	     if(recMsg[4] == theRing.rID)
	       {
		 unsigned char theMessage[numbytes - 6];
		 for(p = 0; p < (numbytes - 7); p++)
		   {
		     theMessage[p] = recMsg[p+6];
		   }
		 theMessage[numbytes - 7] = '\0';
		 printf("\nReceived Message: %s\n", theMessage);
	       }
	     else if(recMsg[4] > 1)
	       {
		 //FORWARD TO NEXT SLAVE
	       }
	     else {
	       printf("Time to Live 1 too low");
	     }
	 
	   }
	 else {
	   printf("Checksum indicates corrupted data");
	 }
      
    }
    
					  

  return 0;
   
    }
}
