CC=g++
RM = rm -rf
INCS=-I.
CFLAGS = -std=c++11 -Wall -g $(INCS) -lpthread
LOADLIBES = -L./
CLIENTSRC = clftp.cpp clftp.h
CLIENTX = clftp
SERVERSRC = srftp.cpp srftp.h
SERVERX = srftp
TAR=tar
TARFLAGS=-cvf
TARNAME=ex5.tar
TARSRCS=srftp.cpp clftp.cpp srftp.h clftp.h Makefile README performance.jpg

all: srftp clftp

clftp: $(CLIENTSRC)
	$(CC) $(CFLAGS) -o $(CLIENTX) $(CLIENTSRC)

srftp: $(SERVERSRC)
	$(CC) $(CFLAGS) -o $(SERVERX) $(SERVERSRC)

clean:
	$(RM) $(CLIENTX) $(SERVERX) *~ *core

tar:
	$(TAR) $(TARFLAGS) $(TARNAME) $(TARSRCS)