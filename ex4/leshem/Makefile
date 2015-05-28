LIBSRC=MyCachingFileSystem.cpp BlockAndCache.hpp 

TAR=tar
TARFLAGS=-cvf
TARNAME=ex5.tar
TARSRCS=$(LIBSRC) Makefile README

all: MyCachingFileSystem

clean:
	rm -f MyCachingFileSystem ioctloutput.log *.o *~
	
MyCachingFileSystem: MyCachingFileSystem.o
	g++ -g -o MyCachingFileSystem MyCachingFileSystem.o `pkg-config fuse --libs`
	
MyCachingFileSystem.o: MyCachingFileSystem.cpp BlockAndCache.hpp 
	g++ -g -Wall `pkg-config fuse --cflags` -c MyCachingFileSystem.cpp
	
tar:
	$(TAR) $(TARFLAGS) $(TARNAME) $(TARSRCS)

ioctl:
	python -c "import os,fcntl; fd = os.open('/tmp/yadid/mount/1.txt', os.O_RDONLY); fcntl.ioctl(fd, 0); os.close(fd)"
	
unmount:
	fusermount -u /tmp/fuse