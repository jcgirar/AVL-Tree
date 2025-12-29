CC = gcc
test: main.o avl.o
	$(CC) -o test main.o avl.o

main.o: main.c avl.h
	$(CC) -c main.c

avl.o: avl.c avl.h
	$(CC) -c avl.c

clean:
	rm test main.o avl.o
