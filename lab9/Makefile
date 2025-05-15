CC=gcc
CFLGAS=-c -g

all: calculator

%.o: %.c
	$(CC) $(CFLGAS) $^ -o $@

calculator: calculator.o
	$(CC) -Wall $^ -o $@

clean:
	rm *.o calculator

test: all
	./calculator 0.000000001 6

.PHONY: all clean calculator test
