NETID := $(shell whoami)

EXES=main
OBJS=ourStats.o internal.o
CFLAGS=-D NETID=\"$(NETID)\"  -D_GNU_SOURCE
DEBUG=-g -Wall

all: $(EXES)

main: $(OBJS)
	gcc -shared -W1,-soname,libstats.so -o libstats.so $^ -lm -lrt -pthread $(DEBUG)

%.o: %.c *.h
	gcc -fPIC -c $< $(CFLAGS) $(DEBUG)

doc: 
	doxygen

clean:
	rm -rf *.o
	rm -rf *.so
	rm -rf $(EXES)
	
.PHONY: doc
