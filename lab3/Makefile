CC=gcc
CFLGAS=-c -g

all: flipper

%.o: %.c
	$(CC) $(CFLGAS) $^ -o $@

flipper: flipper.o
	$(CC) $^ -o $@

clean:
	rm *.o flipper

.PHONY: all clean flipper
