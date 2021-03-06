CC=gcc
CFLAGS=-Wall -I./include -g

example1: example1.o tmax_pmem.o
	$(CC) $(CFLAGS) -o example1 example1.o tmax_pmem.o

example2: example2.o tmax_pmem.o
	$(CC) $(CFLAGS) -o example2 example2.o tmax_pmem.o

clean:
	rm -f example1 example2 *.o