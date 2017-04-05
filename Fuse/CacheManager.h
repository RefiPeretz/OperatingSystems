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
#define SUCCESS 0
#define ERROR -1





using namespace std;

/**
* class block. contain data and block father
*/



class CacheManager
{
public:
	/**
	* Discription: destructor
	*/
	~CacheManager();
	/**
	* Discription: constructor
	*/
	CacheManager(char* root, int numberOfBlock, int blockSizeStandard);
	std::list<Block*> blockList; 
	/**
	* Discription: ger the path of root dir
	*/
	char* getRootDir();
	/**
	* Discription: write to log file
	*/
	int writeToLog(std::string func);
	/**
	* Discription: get the real size of the file
	*/
	int getStandartdSize();
	/**
	* Discription: help function to cache read
	*/
	int cacheRead(int fd, const char* path, off_t offset, size_t sizeToRead, char* buf,\
				  struct fuse_file_info *fi);
	/**
	* Discription: writing to the buffer
	*/
	int bufferWrite(size_t size, off_t offset, std::string pathName, char* buffer);
	/**
	* Discription: finding the block to use next
	*/
	Block* findMyBlock(string name, int position);
	/**
	* Discription: add block to the cache
	*/
	int addBlockToCache(int fd, const char* path, int positionBlock, Block*& newBlock);
	/**
	* Discription: finding the last recent used block
	*/
	Block* leastRecentUsed();
	/**
	* Discription: sort function for the list
	*/
	bool sortFunction(Block* a, Block* b);
	/**
	* Discription: clrearing space in the cache
	*/
	void makeRoomInCache();
	/**
	* Discription: open the log file
	*/
	int initLog();
	/**
	* Discription: rename a file/directory
	*/
	void rename(std::string oldName , std::string newName);
	/**
	* Discription: help function to ioctl number 2
	*/
	std::string toString();
private:
	char _fsPath[PATH_MAX];
	std::fstream _fs;
	char* _rootDir;
	unsigned int _cacheSize;
	int _blockSize;



};

#endif