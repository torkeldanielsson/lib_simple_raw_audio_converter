OUTPUTNAME = test.exe

CC = g++
CCFLAGS = -Wall -std=c++11

OBJS = test.o

default: all

all: $(OBJS)
	$(CC) $(CCFLAGS) -o $(OUTPUTNAME) $(OBJS)

debug: $(OBJS)
	$(CC) $(CCFLAGS) -g -o $(OUTPUTNAME) $(OBJS)

opt: $(OBJS)
	$(CC) $(CCFLAGS) -O3 -o $(OUTPUTNAME) $(OBJS)

.PHONY: clean

clean:
	rm *.o
	rm $(OUTPUTNAME)