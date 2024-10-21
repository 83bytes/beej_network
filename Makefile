CC = gcc
CFLAGS = -g
RM = rm -f

001: 001.c
	$(CC) $(CFLAGS) -o a_001 001.c


showip: showip.c
	$(CC) $(CFLAGS) -o a_showip showip.c