/*
** pollserver.c -- a cheezy multiperson chat server
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
#include <poll.h>

#define PORT "9034"

// Get sockaddr, IPv4 and IPv6;

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET ) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
};

// Return a listening socket
int get_listener_socket(void)
{
    int listener;    // Listening socket descriptor
    int yes=1;       // For setsockopt() SO_REUSEADDR, below
    int rv;

    struct addrinfo hints, *ai, *p;

    // Get us a socket and bind it
    memset (&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
        fprintf(stderr, "pollserver: %s\n", gai_strerror(rv));
        exit(1);
    }

    for (p = ai; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p -> ai_protocol);

        if (listener < 0) {
            continue;
        }

        // lose the pesky "address already in use" error message
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }
        break;
    }

    freeaddrinfo(ai);

    // if p is null, then we didnt get bound
    if (p == NULL) {
        return -1;
    }

    if (listen(listener, 10) == -1) {
        return -1;
    }

    return listener;
}


// Adds a new fd to the set
void add_to_pfds(struct pollfd *pfds[], int newfd, int *fd_count, int *fd_size) 
{
    // If we dont have rooom, add more space in the pfds array
    if (*fd_count == *fd_size) {
        *fd_size *=2; // double it
        
        // dereferencing pfds twice so that we can get the size of each individual pfd
        *pfds = realloc(*pfds, sizeof(**pfds) * (*fd_size));
    }

    // add a new entry at fd_count location
    (*pfds)[*fd_count].fd = newfd;
    (*pfds)[*fd_count].events = POLLIN;

    (*fd_count)++;
}

// remove an index from the list
void del_from_pfds(struct pollfd pfds[], int i, int *fd_count)
{
    // Copy the last entry into this one
    pfds[i] = pfds[*fd_count-1];

    (*fd_count)--;
}

// Main
int main(void)
{
    int listener;

    int newfd;  // newly accepted socket descriptor
    struct sockaddr_storage remoteaddr;  // client address
    socklen_t addelen;

    char buf[256]; // for client data

    char remoteIP[INET6_ADDRSTRLEN];

    // Start off with room for 5 connections
    // we will realloc as needed

    int fd_count = 0;
    int fd_size = 5;

    struct pollfd *pfds = malloc(sizeof *pfds *  fd_size);

    // set up and get a listening socket

    listener = get_listener_socket();

    if (listener == -1) {
        fprintf(stderr, "error getting listening socket \n");
        exit(1);
    }

    // add the listener to the set
    pfds[0].fd = listener;
    pfds[0].events = POLLIN;  // report ready to read on incoming connections

    fd_count = 1;

    // Main loop
    for (;;) {
        int poll_count = poll(pfds, fd_count, -1);

        if (poll_count == -1) {
            perror("poll");
            exit(1);
        }

        // run through the list of fds in the list and check if there is any data to read
        for(int i = 0; i < fd_count; i++) {

            // check if someone is ready to read
            if (pfds[i].revents & POLLIN) {  // as we are checking for POLLIN; AND-ing with itself will always be true

                if (pfds[i].fd == listener ) {
                    // if the listener is ready to read, that means we have a new client
                    // so we should handle the new client

                    int addrlen = sizeof remoteaddr;
                    newfd = accept(listener,
                    (struct sockaddr *)&remoteaddr,
                    &addrlen);

                    if (newfd == -1) {
                        perror ("accept");
                    } else {
                        add_to_pfds(&pfds, newfd, &fd_count, &fd_size);

                        printf("pollserver: new connection from %s on socket %d \n",
                        inet_ntop(remoteaddr.ss_family,
                        get_in_addr((struct sockaddr*)&remoteaddr),
                        remoteIP, INET6_ADDRSTRLEN),
                        newfd);
                    }
                } else {
                // we are not a listener, so just a regular client
                    int nbytes = recv(pfds[i].fd, buf, sizeof buf, 0);

                    int sender_fd = pfds[i].fd;

                    if (nbytes <=0) {
                        // some error
                        if (nbytes == 0) {
                            // connection closed; hence no data recvd
                            printf("pollserver: socket %d hung up \n", sender_fd);
                        } else {
                            perror("recv");
                        }

                        close(pfds[i].fd);  // Close as we really do not need this fd anymore

                        del_from_pfds(pfds, i ,  &fd_count);
                    } else {
                        // we have got some data from the client

                        for(int j = 0; j < fd_count; j++) {
                            //send to everyone!
                            int dest_fd = pfds[j].fd;

                            // Except the listener and ourselves
                            if (dest_fd != listener && dest_fd != sender_fd) {
                                if (send(dest_fd, buf, nbytes, 0) == -1) {
                                    perror("send");
                                }
                            }
                        }
                    } // END send data to everyone
                } // END we are a regular client
            } // END We got a ready-to-read from poll()
        }// END Loop through FDs

    } // END for(;;)

    return 0;
}