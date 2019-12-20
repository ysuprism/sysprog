#Makefile

bufcache: main.o buf.o command.o
	gcc -o bufcache main.o buf.o command.o

main.o: main.c buf.h command.h extern.h
	gcc -c main.c

buf.o: buf.c buf.h extern.h
	gcc -c buf.c

command.o: command.c command.h extern.h
	gcc -c command.c

clean: 
	\rm bufcache *.o
