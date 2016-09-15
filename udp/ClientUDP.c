
/*
** ClientUDP.c -- a datagram "client" demo
*/

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

#define SERVERPORT "10010"	// the port users will be connecting to
#define TENRADIX 10
int main(int argc, char *argv[])
{
	int sockfd;
	int userInputs[5];
	int portNumber;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;

	if (argc <= 3) {
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
	for(z = 3; z < argc; z++){
        if ((numbytes = sendto(sockfd, argv[z], strlen(argv[z]), 0, p->ai_addr, p->ai_addrlen)) == -1) {
            perror("talker: sendto");
            exit(1);
        }
	}
	freeaddrinfo(servinfo);

	printf("talker: sent %d bytes to %s\n", numbytes, argv[1]);
	close(sockfd);
	return 0;
}

int getIntParam(int loc, const char *argv[]){
    printf("talker: getting int param");
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
    printf("talker: got int param");
    return retInt;
}
