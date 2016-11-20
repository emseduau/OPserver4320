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
char nextPort[5];
struct ring theRing;
struct ring
{
  uint8_t mastGID;
  int16Store magic;
  uint8_t rID;
  unsigned char nextSlave[8];
  
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

  sprintf(ringInfo->nextSlave, "%d.%d.%d.%d", response[4], response[5], response[6], response[7]);
}


void makeRequest(unsigned char * theBytes){
  theBytes[0] = groupID;
  theBytes[1] = 0x12; // 18 in decimal.
  theBytes[2] = 0x34; // 52 in decimal.
}



void getPort(char *port, int portNum)
{
  char temp[6];
  sprintf(temp, "%d", portNum);
  puts(temp);
  int i;
  for(i = 0; i < 5; i++)
    {
      port[i] = temp[i];
    }
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

void *prompt(void	*threadid)
{
  int	msgSock,	rslt;
  uint8_t ringID;
  uint8_t ttl = 10;
  unsigned char temp[64];
  
  int	sockudp,	new_fd;
  socklen_t	addr_size;
  struct	addrinfo	udphints, *udpres;
  struct	sockaddr_storage their_addr;
  
  while(1)
	 {
      int	packLength = 7;
		memset(temp,NULL,64); // fill	the memory space will NULL(so	we	can find	end)
 
      
		printf("\nWhat ring ID would you like to send a message? ");
		scanf(" %hhu", &ringID);
		printf("Enter Message: ");
		scanf(" %[^\n]%*c",temp);
      
		int end = 0;
		while(temp[end] != NULL) //find how	long message was but	finding first NULL
	{
	  end	= end	+ 1;
	}
		printf("Length of message: %d\n", end);
		packLength = packLength	+ end;
		unsigned	char packBytes[packLength];
		unsigned	char message[end]; // array for specific size of message
		int x;
		for(x	= 0; x <	end; x++)
	{
	  message[x] =	temp[x];	//transfer message into	new array
	}
   
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
      memset(&udphints, 0, sizeof udphints);
      udphints.ai_family = AF_UNSPEC;
      udphints.ai_socktype = SOCK_DGRAM;
      udphints.ai_flags = AI_PASSIVE; 
      
      packBytes[0] = groupID;
      packBytes[1] = 0x12; // 18 in decimal.
      packBytes[2] = 0x34; // 52 in decimal.
      packBytes[3] = ttl;
      packBytes[4] = ringID;
      packBytes[5] = theRing.rID; //Source Ring ID
      for(x=0; x<end; x++)
      {
         packBytes[6 + x] = message[x];
      }

      packBytes[packLength - 1] = 0;
      packBytes[packLength - 1] = checksum(packBytes, packLength);  
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
      
      
      int portnum = 10010 + (theRing.mastGID * 5) + theRing.rID - 1;

      
      sprintf(nextPort,"%d", portnum);
      
      fprintf("Dest ID: %d\n", packBytes[4]);
      if((rslt = getaddrinfo(theRing.nextSlave, nextPort, &udphints, &udpres)) != 0)
      {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rslt));
		return 1;
	   }
      sockudp = socket(udpres->ai_family,udpres->ai_socktype, udpres->ai_protocol);
      rslt = sendto(sockudp, packBytes, packLength, 0, udpres->ai_addr, udpres->ai_addrlen);
      
      
      close(sockudp);
      
}
}

void forward(char *msg[])

{
 
            
            
            int fwdSock,fwdRes, fwdCheck;
            struct addrinfo fwdhints, *fwdinfo;
            
            memset(&fwdhints, 0 , sizeof fwdhints);
            fwdhints.ai_family = AF_UNSPEC;
            fwdhints.ai_socktype = SOCK_DGRAM;
            fwdhints.ai_flags = AI_PASSIVE;
            
            int fwdport = 10010 + (theRing.mastGID * 5) + theRing.rID - 1;
            
            char portfwd[5];
            
            sprintf(portfwd, "%d", fwdport);
            
            if((fwdCheck = getaddrinfo(theRing.nextSlave, portfwd, &fwdhints, &fwdinfo)) != 0) {
               fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(fwdCheck));
               return 1;
            }
            
            fwdSock = socket(fwdinfo->ai_family, fwdinfo->ai_socktype, fwdinfo->ai_protocol);
            printf("SOCKET: %d\n", fwdSock);
            fwdCheck = sendto(fwdSock, msg,sizeof msg, 0, fwdinfo->ai_addr, fwdinfo->ai_addrlen);
            printf("Sendto: %d\n", fwdCheck);
            close(fwdSock);
            

}

int main(int argc, char *argv[])
{
int sockfd, result, bytes_sent;
  struct addrinfo hints;
  struct addrinfo *res;
  unsigned char req[3];
  unsigned char response[8];

  memset(&hints, 0, sizeof hints); // make sure the struct is empty
  hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
  hints.ai_flags = AI_PASSIVE;     // fill in my IP for me


  result = getaddrinfo(argv[1], argv[2], &hints, &res);
  
  
  
  sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  connect(sockfd, res->ai_addr, res->ai_addrlen);
    
    makeRequest(req);
    send(sockfd, req, 3, 0);
    recv(sockfd, response, 8, 0);
    makeRing(&theRing, response);
    printf("\nGroup ID of Master: %d\n", theRing.mastGID);
    printf("Ring ID is: %d\n", theRing.rID );
    printf("IP of next slave: %s\n", theRing.nextSlave);
   fflush(stdout);
   int check;
  int ONE = 1;
  int zero = 0;
  pthread_t threads[2];
  check = pthread_create(&threads[0], NULL, prompt, (void *)0);
  //printf("CHECKPOINT 1\n");
   
   
   
   
  //CHECK FOR MESSAGE
  
    int sockUDP, new_fd, rslt, numbytes;
    socklen_t addr_len;
    struct addrinfo udphints, *udpres, *p;
    struct sockaddr_storage their_addr;
    unsigned char buf[75];

    int portNum = 10010 + (theRing.mastGID * 5) + theRing.rID;
    char port[5];

    getPort(port, portNum);
    printf("\nPort to bind: %s\n", port);
    
    memset(&udphints, 0, sizeof udphints); // make sure the struct is empty
    udphints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
    udphints.ai_socktype = SOCK_DGRAM; 
    udphints.ai_flags = AI_PASSIVE;     // fill in my IP for me


    if((rslt = getaddrinfo(NULL,port, &udphints, &udpres)) != 0) {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rslt));
		return 1;
    }


   for(p = res; p != NULL; p = p->ai_next) {
    if((sockUDP = socket(udpres->ai_family,udpres->ai_socktype,udpres->ai_protocol)) == -1)
    {
      perror("listener: socket");
      continue;
    }

    if((bind(sockUDP, udpres->ai_addr, udpres->ai_addrlen)) == -1)
    {
      close(sockUDP);
      perror("listener: bind");
      continue;
    }
    
    break;
    }

    addr_len = sizeof their_addr;

    while(1) {
    //printf("listener: waiting to recvfrom...\n");
	 if ((numbytes = recvfrom(sockUDP, buf, 74,0,(struct sockaddr *)&their_addr, &addr_len)) == -1) {
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
	      else{
            printf("\nFORWARDING PACKET...\n");
            
            forward(recMsg);



	       }
	   }
	 else {
	   printf("Checksum indicates corrupted data");
	 }
      
	}
  
 
 //pthread_join(threads[0], NULL);
 
 return check;
  
}




