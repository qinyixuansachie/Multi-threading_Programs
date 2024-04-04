CC=gcc-13
CFLAGS=-fopenmp

all: q2

q2: q2.c
	$(CC) $(CFLAGS) -o q2 q2.c

clean:
	rm -f q2