CC = gcc
CFLAGS = -Wall -Wextra -std=c11
OBJS = main.o server.o http.o

all: server

server: $(OBJS)
	$(CC) -o server $(OBJS)

clean:
	rm -f *.o server
