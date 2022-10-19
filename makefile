CC     = g++
CFLAGS = -DVERBOSE_LOGIN -Wall -Werror -Wextra -Wno-unused-parameter -Wno-unused-variable -g -std=c++0x
LFLAGS = -pthread
TARGETS = server client

%.o: %.cpp %.h
	$(CC) -c $(CFLAGS) $< -o $@

%.o: %.cpp
	$(CC) -c $(CFLAGS) $< -o $@

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

all:  $(TARGETS)

server: server.o common.o log.o
	$(CC) -o server $^ $(LFLAGS)	

server.o: server.cpp common.h
	$(CC) $(CFLAGS) -c $^


client: client.o common.o log.o
	$(CC) -o client $^ $(LFLAGS)	

client.o: client.cpp common.h
	$(CC) $(CFLAGS) -c $^
	
clean:
	-rm *.o core* *.gch $(TARGETS)
	
