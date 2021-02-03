CC=g++
CFLAGS= -g -W -Wall -Wextra -I./include -pthread -lssl -lcrypto
BINSC=client
BINSM=manager
OBJSC=client.o \
	src/criptography.o \
	src/clientprotocol.o \
	src/managerprotocol.o \
	src/socks.o \
	src/tcp.o \
	src/threadpool.o
OBJSM= manager.o \
	src/criptography.o \
	src/tcp.o \
	src/threadpool.o

all: $(BINSC) $(BINSM)

client: $(OBJSC)
	$(CC) -o $@ $^ $(CFLAGS)

manager: $(OBJSM) 
	$(CC) -o $@ $^ $(CFLAGS)

%.o: %.cpp
	$(CC) -c -o $@ $^ $(CFLAGS)

clean:
	rm -rf $(BINSC) $(BINSM) $(OBJSC) $(OBJSM)
