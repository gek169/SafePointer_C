CC= gcc
CFLAGS= -O3

all:
	$(CC) $(CFLAGS) safemem.c -o safemem.out -lpthread -lm

clean:
	rm -f *.exe *.out *.o
