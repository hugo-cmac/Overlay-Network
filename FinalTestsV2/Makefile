CC=g++
CFLAGS=-W -Wall -Wextra -O2 -I./include -pthread -L/usr/lib -lssl -lcrypto
CFLAGS2=-W -Wall -Wextra -O2 -pthread
BINS=client
BINS2=manager
OBJS=client.o \
	src/clientprotocol.o \
	src/managerprotocol.o \
	src/socks.o \
	src/tcp.o \
	src/threadpool.o
	
OBJS2= manager.o

all: $(BINS) $(BINS2)

client: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.cpp
	$(CC) $(CFLAGS) -c -o $@ $^
	
manager:$(OBJS2) 
	$(CC) $(CFLAGS2) -o $@ $^
	
manager.o: manager.cpp
	$(CC) $(CFLAGS) -c -o $@ $^

clean:
	rm -rf $(BINS) $(BINS2) $(OBJS) $(OBJS2)
