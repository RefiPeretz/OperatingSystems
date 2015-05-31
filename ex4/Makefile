CC = g++
CFLAGS = -std=c++0x -g -Wall

EXEC = CachingFileSystem
LIBS = -lfuse
SOURCES = CachingFileSystem.cpp Block.cpp CacheManager.cpp

FUSE_FLAGS = -D_FILE_OFFSET_BITS=64 `pkg-config fuse --cflags --libs`

all:
	$(CC) $(CFLAGS) $(FUSE_FLAGS) -o $(EXEC) $(SOURCES) $(LIBS)
	
tar:
	tar -cvf ex4.tar CachingFileSystem.cpp Block.h Block.cpp \
	CacheManager.cpp CacheManager.h README Makefile

clean:
	rm -rf ./CachingFileSystem ex4.tar

.PHONY = all clean