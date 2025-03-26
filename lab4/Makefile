CC=gcc
CFLGAS=-c -g

all: create_procs

%.o: %.c
	$(CC) $(CFLGAS) $^ -o $@

create_procs: create_procs.o
	$(CC) -Wall $^ -o $@

clean:
	rm *.o create_procs

.PHONY: all clean create_procs
