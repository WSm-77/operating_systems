CC=gcc
CFLGAS=-c -g

all: define_trap_action

%.o: %.c
	$(CC) $(CFLGAS) $^ -o $@

define_trap_action: define_trap_action.o
	$(CC) -Wall $^ -o $@

clean:
	rm *.o define_trap_action

.PHONY: all clean define_trap_action
