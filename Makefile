
CC = gcc
CFLAGS = -O2 -fopenmp

OBJS = ecosystem.o main.o

.PHONY: all clean

all: ecosystem

ecosystem: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

ecosystem.o: ecosystem.c ecosystem.h
	$(CC) $(CFLAGS) -c ecosystem.c -o ecosystem.o

main.o: main.c ecosystem.h
	$(CC) $(CFLAGS) -c main.c -o main.o

clean:
	rm -f *.o ecosystem
