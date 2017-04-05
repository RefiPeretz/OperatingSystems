CC= g++
CFLAGS = -Wall

OSMLIB = libosm.a

TAR=tar
RM=rm
TARFLAGS = -cvf
TARNAME = ex1.tar
TARSRCS = osm.cpp Makefile README

tar: libosm.a
	$(TAR) $(TARFLAGS) $(TARNAME) $(TARSRCS)

all: $(OSMLIB)

libosm.a: osm.o
	ar -rs $(OSMLIB) osm.o

osm.o: osm.cpp osm.h
	$(CC) $(CFLAGS) -c osm.cpp

clean:
	$(RM) -rf $(OSMLIB) osm.o

.PHONY: all clean tar
