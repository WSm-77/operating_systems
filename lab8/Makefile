CC=gcc
CFLAGS=-Wall -pthread -lrt
OUTPUT_FILE=output.txt

all: printer

%.o: %.c
	$(CC) -c $^ -o $@

printer: printer.o
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f printer *.o $(OUTPUT_FILE)

test: all
	@echo "\nTESTING\n"
	@printers=3; \
	users=3; \
	timeout --signal=SIGINT 11s ./printer $$users $$printers | tee $(OUTPUT_FILE) || echo "Timeout reached..."; \
	echo "\n\n####### read output #######\n\n"; \
	for printerID in $$(seq 0 $$(($$printers-1))); do \
		echo "Printer $$printerID:\n"; \
		grep -i "printer$$printerID" $(OUTPUT_FILE); \
	done
