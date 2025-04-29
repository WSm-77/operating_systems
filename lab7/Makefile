CC=gcc
CFLGAS=-c -g

all: client server

%.o: %.c
	$(CC) $(CFLGAS) $^ -o $@

client: client.o
	$(CC) -Wall $^ -o $@

server: server.o
	$(CC) -Wall $^ -o $@

clean:
	rm *.o client server

.PHONY: all clean client server
