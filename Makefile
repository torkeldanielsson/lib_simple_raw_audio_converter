OUTPUTNAME=test.exe

CPP=g++

OBJS=test.o

default: all

all: $(OBJS)
	$(CPP) -o $(OUTPUTNAME) $(OBJS)

debug: $(OBJS)
	$(CPP) -g -o $(OUTPUTNAME) $(OBJS)

opt: $(OBJS)
	$(CPP) -O3 -o $(OUTPUTNAME) $(OBJS)

.PHONY: clean
	
clean:
	rm *.o
	rm $(OUTPUTNAME)