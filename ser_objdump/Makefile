
all: ser_objdump 

ser_objdump: ser_objdump.c select.c select.h stack.c stack.h str.h ser_objdump.h Makefile
	gcc -Wall -g -o ser_objdump -lbfd ser_objdump.c select.c stack.c

.PHONY: clean
clean:
	rm -f ser_objdump
