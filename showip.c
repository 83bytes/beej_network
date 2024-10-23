/*
** showip.c -- show IP address foro a host given on the command line
*/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

int main(int argc, char *argv[]) 
{
    // we create 3 structs.
    // hints is to pass aruments to the getaddrinfo function.
    // res is to hold the result. we pass it as a pointer to the getaddrinfo function.
    // and p is to iterate over the resutls of res
    struct addrinfo hints, *res, *p;
    int status;
    char ipstr[INET6_ADDRSTRLEN];

    if (argc != 2) { // check if argument count is 2
        fprintf(stderr, "usageL showip hostname\n");
        return 1;
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

//  Very imp function
// used to fill up the addrinfo struct with data
// by using the hints struct we can either make this work for a listening socket connection or a sending sicker conn
    if ((status = getaddrinfo(argv[1], NULL, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 2;
    } 

    printf("IP addressess for %s:\n\n", argv[1]);

    // simple traversal of a linked-list. 
    // we go thru the res addrinfo struct which can link to other addrinfo structs.
    // we stop when the next link is NULL
    for(p = res; p != NULL; p = p->ai_next) {
        void *addr;
        char *ipver;

        // get the pointer to the address itself,
        // different fields in IPv4 and IPv6:
        if (p->ai_family == AF_INET) {
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)p -> ai_addr;
            addr = &(ipv4 -> sin_addr);
            ipver = "IPV4";
        } else {
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            addr = &(ipv6 -> sin6_addr);
            ipver = "IPV6";
        }

        // convert the IP to  a string and print it
        inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
        printf(" %s: %s\n", ipver, ipstr);
    }

    freeaddrinfo(res);

    return 0;
}