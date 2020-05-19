CC=gcc

client : client.o
	$(CC) -o client client.o

client.o : client.c
	$(CC) -c client.c

server : server.o
	$(CC) -o server server.o -lpthread

server.o : server.c
	$(CC) -c server.c -lpthread

clean:
	rm client client.o server server.o

