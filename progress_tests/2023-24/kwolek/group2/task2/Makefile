CC=gcc
CFLAGS=-c

*.o: *.c
	$(CC) $(CFLAGS) -o $@ $<

main: main.o
	$(CC) -o $@ $<

test: main
	for i in {0..5}; do \
		./main $$i; \
		echo "fib($$i)=$$?"; \
	done

all: main

clean: rm *.o main

.PHONY: all clean test
