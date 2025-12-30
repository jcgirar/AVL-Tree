CC = gcc
CFLAGS = -std=c99

test: main.o avl.o
	$(CC) $(CFLAGS) -o test main.o avl.o

main.o: main.c avl.h
	$(CC) $(CFLAGS) -c main.c

avl.o: avl.c avl.h
	$(CC) $(CFLAGS) -c avl.c

clean:
	rm test main.o avl.o
