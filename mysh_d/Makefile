#Makefile

mysh: main.o utils.o
	gcc -g -O0 -o mysh main.o utils.o

main.o: main.c token.h
	gcc -g -O0 -c main.c

utils.o: utils.c token.h
	gcc -g -O0 -c utils.c

clean: 
	\rm mysh *.o
