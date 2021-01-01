CC=g++
CFLAGS=-W -Wall -Wextra -O2 -I./include -pthread -L/usr/lib -lssl -lcrypto
BINS=client
OBJS=client.o \
	src/clientprotocol.o \
	src/managerprotocol.o \
	src/socks.o \
	src/tcp.o \
	src/threadpool.o

all: $(BINS)

client: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.cpp
	$(CC) $(CFLAGS) -c -o $@ $^

clean:
	rm -rf $(BINS) $(OBJS)