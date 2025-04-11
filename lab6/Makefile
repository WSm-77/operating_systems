CC=gcc
CFLGAS=-c -g

all: calculator calculation_controller parent_child_communication

%.o: %.c
	$(CC) $(CFLGAS) $^ -o $@

parent_child_communication: parent_child_communication.o
	$(CC) -Wall $^ -o $@

calculator: calculator.o
	$(CC) -Wall $^ -o $@

calculation_controller: calculation_controller.o
	$(CC) -Wall $^ -o $@

clean:
	rm *.o parent_child_communication calculator calculation_controller named_pipe

.PHONY: all clean parent_child_communication calculator calculation_controller
