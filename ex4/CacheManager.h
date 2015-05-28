/**
* CacheManager.h
* Date: 21/5/2015
*/
#ifndef _CACHEMANAGER_H
#define _CACHEMANAGER_H

#include <ctime>
#include <iostream>
#include "Block.h"
#include <fstream>
#include <list>





using namespace std;

/**
* class block. contain data and block father
*/



class CacheManager
{
public:
	~CacheManager();
	CacheManager(char* root, char* mount, int numberOfBlock, int blockSizeStandard);
	std::list<Block*> blockList; 
	char* getRootDir();
	void writeToLog(std::string func);
	int getStandartdSize();
	int cacheRead(int fd, const char* path, off_t offset, size_t sizeToRead, char* buf);
	int bufferWrite(size_t size, off_t offset, std::string pathName, char* buffer);
	Block* findMyBlock(string name, int position);
	int addBlockToCache(int fd, const char* path, int positionBlock, Block*& newBlock);
	Block* leastRecentUsed();
	bool sortFunction(Block* a, Block* b);
	void makeRoomInCache();
	void Rename(std::string oldName ,std::string newName);
	void Ioctl(const char *, int cmd, void *arg,
		struct fuse_file_info *, unsigned int flags, void *data_);
	std::string toString();

private:
	char _fsPath[PATH_MAX];
	std::fstream _fs;
	char* _rootDir;
	unsigned int _cacheSize;
	int _blockSize;



};

#endif