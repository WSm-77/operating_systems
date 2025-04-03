CC=gcc
CFLGAS=-c -g

all: define_trap_action catcher sender

%.o: %.c
	$(CC) $(CFLGAS) $^ -o $@

define_trap_action: define_trap_action.o
	$(CC) -Wall $^ -o $@

catcher: catcher.o
	$(CC) -Wall $^ -o $@

sender: sender.o
	$(CC) -Wall $^ -o $@

clean:
	rm *.o define_trap_action catcher sender

.PHONY: all clean define_trap_action
