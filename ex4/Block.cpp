/**
* Block.cpp
* Date: 21/5/2015
*/
#include "Block.h"

/**
* Discription: defulte constructor
* @param: 
* @return: nothing
*/
Block::Block(string file, long offset, char* newData, int position)
{
	_blockFileName = file;
	_blockStartOffset = offset;
	_blockData = newData;
	_readCounter = 0;
	_blockPosition = position;

}

/**
* Discription: get function
* @param: 
* @return: block size
*/
int Block::getBlockSize()
{
	return _blockSize;
}

/**
* Discription: get function
* @param: 
* @return: block data
*/
char* Block::getBlockData()
{
	return _blockData;
}

/**
* Discription: get function
* @param: 
* @return: counter
*/
int Block::getCounter()
{
	return _readCounter;
}

/**
* Discription: set's the block data
* @param: char* newData
* @return: 
*/
void Block::setBlockData(char* newData)
{
	_blockData = newData;
}

/**
* Discription: add 1 to block counter
* @param: 
* @return:
*/
void Block::addToCounter()
{
	_readCounter++;
}

/**
* Discription: get function
* @param: 
* @return: counter
*/
string  Block::getBlockFileName()
{
	return _blockFileName;
}

/**
* Discription: get function
* @param: 
* @return: block name
*/
void Block::setBlockFileName(string newName)
{
	_blockFileName = newName;
}

/**
* Discription: get function
* @param: 
* @return: block Start Offset
*/
long Block::getBlockStartOffset()
{
	return _blockStartOffset;
}

/**
* Discription: destractor
* @param: 
* @return: 
*/
Block::~Block()
{
	delete[](_blockData);

}

/**
* Discription: get function
* @param: 
* @return: the block number for the file
*/
int Block::getBlockPosition()
{
	return _blockPosition;
}

/**
* Discription: set the position of the block 
* @param: 
* @return: 
*/
void Block::setBlockPosition(int newPosition)
{
	_blockPosition = newPosition;
}

/**
* Discription: set the position of the block 
* @param: std::string rootpath
* @return: 
*/
std::string Block::toString(std::string rootpath)//TOTO change parameters
{
	std::string rp = rootpath;
	std::string filename = _blockFileName;
	std::string relativeName = filename.substr(rp.length() + 1);
	std::stringstream ss;
	ss << relativeName << " " << _blockPosition << " " << _readCounter << endl;
	return ss.str();
}







