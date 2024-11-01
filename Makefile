CC = gcc
CFLAGS = -g
RM = rm -f

001: 001.c
	$(CC) $(CFLAGS) -o a_001 001.c


showip: showip.c
	$(CC) $(CFLAGS) -o a_showip showip.c


client_1: client_1.c
	$(CC) $(CFLAGS) -o a_client_1 client_1.c

server_1: server_1.c
	$(CC) $(CFLAGS) -o a_server_1 server_1.c

poll_1: poll_1.c
	$(CC) $(CFLAGS) -o a_poll_1 poll_1.c

pollserver: pollserver.c
	$(CC) $(CFLAGS) -o a_pollserver pollserver.c