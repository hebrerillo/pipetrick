CC     = g++
CFLAGS = -DVERBOSE_LOGIN -Wall -Werror -Wextra -Wno-unused-parameter -Wno-unused-variable -g -std=c++0x
LFLAGS = -pthread
GTEST_LIB = lib/libgtest.a
TARGETS = main test

%.o: %.cpp %.h
	$(CC) -c $(CFLAGS) $< -o $@

%.o: %.cpp
	$(CC) -c $(CFLAGS) $< -o $@

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

all:  $(TARGETS)	

server.o: server.cpp server.h log.h common.h
	$(CC) $(CFLAGS) -c $^	

client.o: client.cpp client.h log.h common.h
	$(CC) $(CFLAGS) -c $^
	
main: main.o server.o client.o log.o common.o
	$(CC) -o main $^ $(LFLAGS)
	
main.o: main.cpp server.h client.h log.h
	$(CC) $(CFLAGS) -c $^

test: test.o server.o client.o log.o common.o
	$(CC) -o test $^ $(LFLAGS) $(GTEST_LIB)

test.o: test.cpp test.h server.h client.h
	$(CC) $(CFLAGS) -c $^

clean:
	-rm *.o core* *.gch main test $(TARGETS)
	
