/**
* Block.h
* Date: 21/5/2015
*/
#ifndef _BLOCK_H
#define _BLOCK_H

#include <iostream>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>
#include <dirent.h>
#include <stdio.h>
#include <queue>
#include <sys/time.h>
#include <algorithm>
#include <fstream>
#include <errno.h>
#include <math.h>
#include <string>
#include <sstream> 


using namespace std;

/**
* class block. contain data and block father
*/
class Block
{
public: 
	/**
	* Discription: constructor
	*/
	Block(string file, long blockNum, char* newData, int position);
	/**
	* Discription: defulte constructor
	*/
	Block();
	/**
	* Discription: destractor
	*/
	~Block();
	/**
	* Discription: get block size
	*/
	int getBlockSize();
	/**
	* Discription: get block data
	*/
	char* getBlockData();
	/**
	* Discription: get block counter
	*/
	int getCounter();
	/**
	* Discription: set block data
	*/
	void setBlockData(char* newData);
	/**
	* Discription: add block to the cache
	*/
	void addToCounter();
	/**
	* Discription: get block name
	*/
	string  getBlockFileName();
	/**
	* Discription: set block name
	*/
	void setBlockFileName(string newName);
	/**
	* Discription: get block offset
	*/
	long getBlockStartOffset();
	/**
	* Discription: make room in the cache
	*/
	void makeRoomInCache();
	/**
	* Discription: get block position
	*/
	int getBlockPosition();
	/**
	* Discription: set block position
	*/
	void setBlockPosition(int newPosition);

	std::string toString(std::string rootpath);

private:
	int _blockSize;
	char* _blockData;
	int _readCounter;
	string _blockFileName;
	long _blockStartOffset;
	struct timeval _lastAccess;
	int _blockPosition;




};

#endif