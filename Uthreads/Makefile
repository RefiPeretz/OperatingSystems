CC=g++


CFLAGS=-Wall -Wextra $(INCS)


AR=ar rvs


TAR=tar -cvf


TARNAME=ex2.tar


TARGETS = uthreads.cpp myThread.cpp manager.cpp

HEADERS = uthreads.h myThread.hpp manager.hpp

LIBUTHREADS = libuthreads.a

TARSRCS=$(TARGETS) Makefile README

all: uthreads.a

%.o: %.cpp
	$(CC) $(CFLAGS) -c $^
uthreads.a: uthreads.o myThread.o manager.o
	$(AR) $(LIBUTHREADS) $^

clean:
	rm -rf uthreads.o uthreads.a

tar:
	$(TAR) $(TARNAME) $(TARSRCS) $(HEADERS)
.PHONY: all
