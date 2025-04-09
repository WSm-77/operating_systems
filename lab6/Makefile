CC=gcc
CFLGAS=-c -g

all: define_trap_action catcher sender

%.o: %.c
	$(CC) $(CFLGAS) $^ -o $@

parent_child_communication: parent_child_communication.o
	$(CC) -Wall $^ -o $@

clean:
	rm *.o parent_child_communication

.PHONY: all clean parent_child_communication
