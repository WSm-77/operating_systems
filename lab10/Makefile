CC=gcc
CFLGAS=-c -g

all: hospital

%.o: %.c
	$(CC) $(CFLGAS) $^ -o $@

hospital: hospital.o
	$(CC) -Wall $^ -o $@

clean:
	rm *.o hospital

test: all
	./hospital 2 3

.PHONY: all clean hospital test
