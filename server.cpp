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

#define PORT "3490" // the port users will be connecting to

#define BACKLOG 10 // how many pending connections queue will hold

#define MAXDATASIZE 141 // the maximum number of bytes of data we can receive at once

void sigchld_handler(int s)
{
    (void)s; // quiet unused variable warning

    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}

// get sock addr, IPv4 or IPv6
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void) {
    // listen on sockfd, new connection on new_fd. fd: file descriptor
    int sockfd, new_fd;
    struct addrinfo hints, *p;
    struct addrinfo *servinfo; // will point to results. is a linked list LOL
    struct sockaddr_storage their_addr; // connector's address info
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN]; // inet means internet :3 space to hold the IPv6 string
    int status;

    memset(&hints, 0, sizeof hints); // ensure hints is empty
    // ai: address info
    hints.ai_family = AF_UNSPEC; // don't care if ipv4 or ipv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_flags = AI_PASSIVE; // fill in my ip for me. 
    /* ^ if you want a specific address, don't fill this flag.
    Instead, type the IP address in getaddrinfo()'s first parameter instead of NULL */

    // error check getaddrinfo: Prepare to launch!
    if((status = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        // Get the file descriptor
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        // lose the pesky "Address already in use" error message
        if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        // What port am I on?
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            perror("server: bind");
            /* Every open socket, file, or pipe uses one entry from a process's file descriptor table, 
            and the OS caps how many a single process can have. So it is best to close failed socket binds */
            close(sockfd);
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    // finished loop w/o binding
    if (p == NULL) {
        fprintf(stderr, "server failed to bind\n");
        exit(1);
    }

    // will somebody plz call me >_< !!
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

    printf("server: waiting for connections. . .\n");

    // main accept loop
    while(1) {
        sin_size = sizeof their_addr;
        // thx for calling port PORT ^_^
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        // failed to accept. . . ok we keep trying!
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        // ntop: network to presentation (printable)
        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
        printf("server: got connection from %s ^_^ !\n", s);

        // Let's fork and see if we get a child process. fork() = 0
        if (!fork()) { // child process returns 0 on success
            // child doesn't need the listener
            close(sockfd);
            if(send(new_fd, "Hello Client, I am da Server!\nSend me something so I can prove to you I am a reliable TCP connection (>//<) !\nPress enter when you're done.", 141, 0) == -1) {
                perror("send");
            }
            
            close(new_fd);
            exit(0); // end child process
        }
        // no child process. . . lets close the new_fd connection.
        close(new_fd); // parent doesn't need this
    }

    return 0;
}