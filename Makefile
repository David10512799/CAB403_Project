#Makefile for Carpark Management System
CC= gcc
CFLAGS= -Wall -Wextra -Werror
LDFLAGS= -lrt -lpthread

all: buildall

./build/manager.o: manager.c carlist.h carpark.h
	$(CC) -c $< -o $@ $(CFLAGS) $(LDFLAGS)

./build/simulator.o: simulator.c carlist.h carpark.h
	$(CC) -c $< -o $@ $(CFLAGS) $(LDFLAGS)

./build/firealarm.o: firealarm.c carpark.h
	$(CC) -c $< -o $@ $(CFLAGS) $(LDFLAGS)
	gcc ./build/firealarm.o -o ./build/firealarm

buildall: ./build/manager.o ./build/simulator.o ./build/firealarm.o

run:
	./build/simulatorm
	./build/manager
	./build/firealarm

clean:
	rm -f ./build/*.o

.PHONY: all clean run buildall