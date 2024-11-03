/*
** select.c -- a select() demo
*/

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define STDIN 0 // file descriptor for standard input

int main(void)
{
    struct timeval tv;
    fd_set readfds;

    tv.tv_sec = 2;
    tv.tv_usec = 500000;

    // these are the macros used to modify / deal with the readfds datastructure
    FD_ZERO(&readfds);
    FD_SET(STDIN, &readfds);

    // dont care about writefds and exceptfds:
    select(STDIN +1, &readfds, NULL, NULL, &tv);
    // when select returns; it modifies the readfds with the fds that need to be read.

    if (FD_ISSET(STDIN, &readfds))
        printf("A key was pressed!\n");
    else
        printf("Timed out. \n");


    return 0;
}