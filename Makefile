#Makefile

mysh: main.o getargs.o
	gcc -g -O0 -o mysh main.o getargs.o

main.o: main.c
	gcc -g -O0 -c main.c

getargs.o: getargs.c
	gcc -g -O0 -c getargs.c

clean: 
	\rm mysh *.o
