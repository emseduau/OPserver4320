/*
** listener.c -- a datagram sockets "server" demo
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

#define MYPORT "10025"	// the port users will be connecting to

#define MAXBUFLEN 100

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{

	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;
	struct sockaddr_storage their_addr;
	char buf[MAXBUFLEN];
	socklen_t addr_len;
	char s[INET6_ADDRSTRLEN];
	if (argc != 2) {
		fprintf(stderr,"usage: listener portNumber\n");
		exit(1);
	}




	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, MYPORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("listener: socket");
			continue;
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("listener: bind");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "listener: failed to bind socket\n");
		return 2;
	}

	freeaddrinfo(servinfo);
	addr_len = sizeof their_addr;
	buf[0] = 1;
	while(buf[0] != 0){
        printf("listener: waiting to recvfrom...\n");
        if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
            perror("recvfrom");
            exit(1);
        }
        if(buf[0] != 0){
            printf("listener: got packet from %s\n",
                inet_ntop(their_addr.ss_family,
                get_in_addr((struct sockaddr *)&their_addr),
                s, sizeof s));
            printf("listener: packet is %d bytes long\n", numbytes);
            buf[numbytes] = '\0';
            printf("listener: packet contains \"%s\"\n", buf);
            printf("listener: Sending response...");
            sendto(sockfd, buf, numbytes, 0, (struct sockaddr *)&their_addr, addr_len);
            printf("listener: Response sent.");
        }
	}
	close(sockfd);

	return 0;
}

int getIntParam(int loc, char *argv[]){
    char *p;
    int num;
    int retInt = 0;
    long conver = strtol(argv[loc], &p, TENRADIX);
    if (errno != 0 || *p != '\0' || conv > INT_MAX)
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





