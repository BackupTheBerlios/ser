
all: ser_objdump 

ser_objdump: ser_objdump.c Makefile
	gcc -Wall -g -o ser_objdump -lbfd ser_objdump.c

.PHONY: clean
clean:
	rm -f ser_objdump
