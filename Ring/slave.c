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


void makeRing(struct ring ringInfo, unsigned char * response)
{
  int16Store magicN;
  ringInfo.mastGID = response[0];
  magicN.theBytes[0] = response[1];
  magicN.theBytes[1] = response[2];
  magicN.theInt = get16FromBytes(magicN.theBytes);
  ringInfo.magic = magicN;
  ringInfo.rID = response[3];
  ringInfo.nextSlave[0] = response[4];
  ringInfo.nextSlave[1] = response[5];
  ringInfo.nextSlave[2] = response[6];
  ringInfo.nextSlave[3] = response[7];
}


void setHints(struct addrinfo *temp)
{
  memset(temp, 0, sizeof temp); // make sure the struct is empty
  temp->ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
  temp->ai_socktype = SOCK_STREAM; // TCP stream sockets
  temp->ai_flags = AI_PASSIVE;     // fill in my IP for me
}

void makeRequest(unsigned char * theBytes){
  theBytes[0] = groupID;
  theBytes[1] = 0x12; // 18 in decimal.
  theBytes[2] = 0x34; // 52 in decimal.
}


int main(int argc, char *argv[])
{
  int sockfd, result, bytes_sent;
  struct addrinfo hints;
  struct addrinfo *res;
  unsigned char req[3];
  struct ring theRing;
  unsigned char response;
  
  setHints(&hints);
  result = getaddrinfo(argv[1], argv[2], &hints, &res);
  
  


  
  while(1) {
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    connect(sockfd, res->ai_addr, res->ai_addrlen);
    makeRequest(&req);
    send(sockfd, req, 3, 0);
    recv(sockfd, response, 8, 0);
    makeRing(theRing, &response);
    close(sockfd);
  }
   
}
