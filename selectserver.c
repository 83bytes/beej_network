/*
** selectverser.c -- a multiperson chat server
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT "9034"

// get sockaddr, IPv4 or IPv6;
void *get_in_addr(struct sockaddr *sa)
{
    if (sa-> sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void)
{
    fd_set master; // master fd list
    fd_set read_fds; // temp fd list (for select())
    int fdmax;

    int listener; // listening socket descriptor
    int newfd; // newly accept()ed socket descriptor
    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;

    char buf[256]; // buffer for client data
    int nbytes;

    char remoteIP[INET6_ADDRSTRLEN];

    int yes=1;
    int i, j, rv;

    struct addrinfo hints, *ai, *p;

    FD_ZERO(&master);
    FD_ZERO(&read_fds);

    // get us a socket and bind it
    memset(&hints, 0 , sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }

    for (p = ai; p!=NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) {
            continue; // keep trying different ai in the result Linkedlist untill were able to listen on a socket
        }

        // lose the "address already in use" error message
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        // now bind
        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }


        break; // stop if we find a listener and we can bind to it
    }

    // if we get here, it means we didnt get bound
    if (p == NULL) {
        fprintf(stderr, "selectserver: failed to bind\n");
        exit(2);
    }

    freeaddrinfo(ai); // all done with this

    // listen
    if (listen(listener, 10) == -1) {
        perror("listen");
        exit(3);
    }

    // add the listener to the master et
    FD_SET(listener, &master);

    // keep track of the biggest fd
    fdmax = listener; // so far this is the one

    // main loop
    for (;;) {
        read_fds = master; // copy it

        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }

        // run through the existing connection looking for data to read
        // read_fds has been modified by select above
        for ( i = 0; i<= fdmax; i++) {
            if(FD_ISSET(i, &read_fds)) { // checking if the fd (i) is in the fd_reads set
                if (i == listener) {
                    // handle new connections
                    addrlen = sizeof  remoteaddr;
                    newfd = accept(listener,
                    (struct scokaddr *)&remoteaddr,
                    &addrlen);

                    if (newfd == -1) {
                        perror("accept");
                    } else {
                        FD_SET(newfd, &master); // add this new connection fd to the master set
                        if (newfd > fdmax) { // keep track of fdmax
                            fdmax = newfd;
                        }

                        printf("selectserver: new connection from %s on socket %d\n",
                                inet_ntop(remoteaddr.ss_family, get_in_addr((struct sockaddr*)&remoteaddr),
                                remoteIP, INET6_ADDRSTRLEN),
                                newfd);
                    } // END accept logic
                } else { // END we are not the listener, 
                    // handle data from a client

                    if (( nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
                        // error or connection closed by client
                        if (nbytes == 0) {
                            // connection closed
                            printf("selectserver: socket %d hung up\n", i);
                        } else {
                            perror("recv");
                        }
                        close(i);
                        FD_CLR(i, &master); // remove from master set as it as closed connection 
                    } else {
                        // no error, we got some data
                        for (j = 0; j <=fdmax; j++) {
                            // send to everyone
                            if (FD_ISSET(j, &master)) { // only send to clients we have connected with
                                // except itself and the listener
                                if (j != listener && j != i ) {
                                    if (send(j, buf, nbytes,0) == -1) {
                                        perror("send");
                                    } // end send
                                }
                            }
                        }
                    } // end sending data
                } // END handle foreign client
            } // END new incoming connection
        } // END looping thru fds
    } // END for(;;)
    return 0;
}