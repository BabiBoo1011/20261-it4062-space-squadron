CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Icommon

all: server_bin client_bin

server_bin: server/main.c common/framing.c
	$(CC) $(CFLAGS) server/main.c common/framing.c -o server_bin

client_bin: client/main.c common/framing.c
	$(CC) $(CFLAGS) client/main.c common/framing.c -o client_bin

clean:
	rm -f *.o server/*.o client/*.o common/*.o server_bin client_bin
