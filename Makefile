CC = gcc
CFLAGS = -c -Wall -Wextra -g

all: server client

server: server.o
	@echo "* Linking server..."
	$(CC) server.o -o server

client: client.o
	@echo "* Linking client..."
	$(CC) client.o -o client

server.o: server.c
	@echo "* Compiling server.c..."
	$(CC) server.c $(CFLAGS)

client.o: client.c
	@echo "* Compiling client.c..."
	$(CC) client.c $(CFLAGS)

clean:
	@echo "Cleaning up..."
	rm -f *.o server client
