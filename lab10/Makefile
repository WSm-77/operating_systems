CC=gcc
CFLGAS=-c -g -pthread

all: hospital

%.o: %.c
	$(CC) $(CFLGAS) $^ -o $@

hospital: hospital.o
	$(CC) -Wall $^ -o $@

clean:
	rm *.o hospital

test: all
	./hospital 9 3

.PHONY: all clean hospital test
