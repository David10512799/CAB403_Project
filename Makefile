#Makefile for Carpark Management System
CC= gcc
CFLAGS= -Wall -Wextra -Werror
LDFLAGS= -lrt -lpthread

all: buildall

./build/manager: manager.c carlist.h carpark.h
	$(CC) $< -o $@ $(CFLAGS) $(LDFLAGS)

./build/simulator: simulator.c carlist.h carpark.h
	$(CC) $< -o $@ $(CFLAGS) $(LDFLAGS)

./build/firealarm: firealarm.c carpark.h
	$(CC) $< -o $@ $(CFLAGS) $(LDFLAGS)

buildall: ./build/manager ./build/simulator ./build/firealarm


run:
	./build/simulatorm
	./build/manager
	./build/firealarm

clean:
	rm -f ./build/*.o

.PHONY: all clean run