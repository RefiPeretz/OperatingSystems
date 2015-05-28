FLAGS = -Wall -g -std=c++11
CPP = g++
FUSE_LIBS = `pkg-config fuse --libs`
FUSE_FLAGS = `pkg-config fuse --cflags` -c
FILE_OFFSET = -D_FILE_OFFSET_BITS=64
TAR=tar
TARFLAGS=-cvf
TARNAME=ex5.tar


all: CachingFileSystem

CachingFileSystem: Block.o CacheManager.o CachingFileSystem.o
	$(CPP) $(FLAGS) -o CachingFileSystem CachingFileSystem.o $(FUSE_LIBS)

CachingFileSystem.o: CachingFileSystem.cpp
	$(CPP) $(FLAGS) $(FUSE_FLAGS) CachingFileSystem.cpp $(FILE_OFFSET)

Block.o: Block.h Block.cpp
	g++ -Wall -Wextra -Wvla -std=c++11 -c Block.cpp

CacheManager.o: CacheManager.h Block.h CacheManager.cpp
	g++ -Wall -Wextra -Wvla -std=c++11 -c CacheManager.cpp

clean:
	rm -f *.o *~ CachingFileSystem ioctloutput.log

.PHONEY: all clean CachingFileSystem

tar:
	$(TAR) $(TARFLAGS) $(TARNAME) CachingFileSystem.cpp Makefile README
