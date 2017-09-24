OUTPUTNAME = test.exe

CC     = gcc
CFLAGS = -std=c++11 -Wall 

OBJS = test.o

default: all

test.o: test.cpp simple_raw_audio_converter.h dr_wav.h
	$(CC) $(CFLAGS) -c test.cpp

all: $(OBJS)
	$(CC) $(CFLAGS) -o $(OUTPUTNAME) $(OBJS)

debug: $(OBJS)
	$(CC) $(CFLAGS) -g -o $(OUTPUTNAME) $(OBJS)

opt: $(OBJS)
	$(CC) $(CFLAGS) -O3 -o $(OUTPUTNAME) $(OBJS)

.PHONY: clean

clean:
	rm -f *.o
	rm -f $(OUTPUTNAME)
	rm -f test_*.wav
	