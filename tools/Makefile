CC     = g++
OPT    = -O2 # -O3
DEBUG  = -g
# OTHER  = -Wall -Wno-deprecated
CFLAGS = $(OPT) $(OTHER)


all: nanoxim_explorer 

nanoxim_explorer: nanoxim_explorer.o
	$(CC) $(CFLAGS) nanoxim_explorer.o -o nanoxim_explorer

nanoxim_explorer.o: nanoxim_explorer.cpp
	$(CC) $(CFLAGS) -c nanoxim_explorer.cpp -o nanoxim_explorer.o

clean:
	rm -f *.o nanoxim_explorer 

