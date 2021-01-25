CC=g++
CFLAGS=-W -Wall -Wextra -I./include -pthread -lssl -lcrypto
CFLAGS2=-W -Wall -Wextra -pthread
BINS=client
BINS2=manager
OBJS=client.o \
	src/criptography.o \
	src/clientprotocol.o \
	src/managerprotocol.o \
	src/socks.o \
	src/tcp.o \
	src/threadpool.o
	
OBJS2= manager.o

all: $(BINS) $(BINS2)

client: $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)

%.o: %.cpp
	$(CC) -c -o $@ $^ $(CFLAGS)
	
manager:$(OBJS2) 
	$(CC) $(CFLAGS2) -o $@ $^
	
manager.o: manager.cpp
	$(CC) $(CFLAGS) -c -o $@ $^

clean:
	rm -rf $(BINS) $(BINS2) $(OBJS) $(OBJS2)
