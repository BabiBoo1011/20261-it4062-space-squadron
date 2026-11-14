CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Icommon

all:
	@echo "San sang build server va client"

clean:
	rm -f *.o server/*.o client/*.o common/*.o server_bin client_bin
