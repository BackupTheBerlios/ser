CC	= gcc
LD	= gcc

wx2000:	wx2000.o
	$(LD) -o wx2000 wx2000.o

wx2000.o: wx2000.c config.h
	$(CC) -c wx2000.c

debug:
	$(CC) -DDEBUG -o wx2000-debug wx2000.c

clean:
	$(RM) wx2000 wx2000.o wx2000-debug core *~

dist:	wx2000.c wx2000.h serial.c config.h Makefile README
	$(RM) wx2000 wx2000.o core wx2000-0.2-Beta.tar.gz *~
	cd .. ; tar -czvf wx2000-0.2-Beta.tar.gz wx2000-0.2-Beta
