#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <iostream>

#define PORT "3490" // client will connect to this port

#define MAXDATASIZE 141 // the maximum number of bytes of data we can receive at once

// get sockaddr, IPv4 or IPv6
void *get_in_address(struct sockaddr *sa) {
    // 4
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    // 6
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[]) {
    int sockfd, numbytes;
    char buf[MAXDATASIZE];

    // hints points to a struct addrinfo that you’ve already filled out with relevant information.
    struct addrinfo hints, *p;
    struct addrinfo *servinfo; // will point to results. is a linked list LOL
    int status;
    char s[INET6_ADDRSTRLEN];

    // ./client localhost = 2
    if (argc != 2) {
        fprintf(stderr,"usage: client hostname\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // ipv4 or 6 is fine
    hints.ai_socktype = SOCK_STREAM; // TCP

    // argv[1] is localhost
    // error check getaddrinfo: Prepare to launch!
    if ((status = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddressinfo: %s\n", gai_strerror(status));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        // Get the file descriptor
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        // network to presentation. get the 
        inet_ntop(p->ai_family, 
            get_in_address((struct sockaddr *)p->ai_addr),
            s, sizeof s);
        printf("client: attempting connection to %s\n", s);

        // hey you !
        if(connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            perror("client: connect");
            close(sockfd);
            continue;
        }

        break;
    }

    // we iterated through the entire results and found NADA...
    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    inet_ntop(p->ai_family,
            get_in_address((struct sockaddr *)p->ai_addr),
            s, sizeof s);
    printf("client: connected to %s\n", s);

    freeaddrinfo(servinfo); // done w this structure since connection got established

    if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
        perror("reecv");
        exit(1);
    }

    buf[numbytes] = '\0';
    printf("client: received '%s'\n", buf);

    close(sockfd);

    return 0;
}