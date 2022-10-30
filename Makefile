#Makefile for Carpark Management System
CC= gcc
CFLAGS= -Wall -Wextra -Werror
LDFLAGS= -lrt -lpthread

all: buildall

./build/manager: manager.c carlist.h carpark.h manager.h
	$(CC) $< -o $@ $(CFLAGS) $(LDFLAGS) $(CPPFLAGS)

./build/simulator: simulator.c carlist.h carpark.h simulator.h
	$(CC) $< -o $@ $(CFLAGS) $(LDFLAGS)	$(CPPFLAGS)

./build/firealarm: firealarm.c carpark.h firelalarm.h
	$(CC) $< -o $@ $(CFLAGS) $(LDFLAGS) $(CPPFLAGS)

buildall: ./build/manager ./build/simulator ./build/firealarm

clean:
	rm -f ./build/*.o

.PHONY: all clean run buildall