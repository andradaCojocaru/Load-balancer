CC=gcc -g
CFLAGS=-std=c99 -Wall -Wextra
LOAD=load_balancer
SERVER=server

.PHONY: build clean

build: tema2

tema2: main.o load_balancer.o server.o LinkedList.o utils.h
	$(CC) $^ -o $@

main.o: main.c
	$(CC) $(CFLAGS) $^ -c

server.o: server.c server.h
	$(CC) $(CFLAGS) $^ -c

load_balancer.o: load_balancer.c load_balancer.h
	$(CC) $(CFLAGS) $^ -c

LinkedList.o: LinkedList.c LinkedList.h
	$(CC) $(CFLAGS) $^ -c

clean:
	rm -f *.o tema2 *.h.gch
