CC=gcc
CFLGAS=-c -g

all: create_procs

%.o: %.c
	$(CC) $(CFLGAS) $^ -o $@

create_procs: create_procs.o
	$(CC) -Wall $^ -o $@

child_procs_exec_ls: child_procs_exec_ls.o
	$(CC) -Wall $^ -o $@

clean:
	rm *.o create_procs child_procs_exec_ls

.PHONY: all clean create_procs child_procs_exec_ls
