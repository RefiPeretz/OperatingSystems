/**
* the cpp file for the class block
*/
#include "blockchain.h"
#include "Block.h"
#include "hash.h"

#define TRUE 1

/**
* Discription: defulte constructor
* @param: int newBlockId, char* newData, int dataLength,Block* blockFather
* @return: nothing
*/
Block::Block(int newBlockId, char* newData, int dataLength, Block* blockFather)
{
	blockId = newBlockId;
	this->data = new char [length];
	strncpy(this->data, newData, length);
	this->dataLength = dataLength;
	this->blockFather = blockFather;
	this->height = getFatherHighet() + 1;
	this->pruneBlock = true;
	this->isLongest = false;
	this->isAttached = false;
}

/**
* Discription: get father id
* @param: 
* @return: the block's fathe id
*/
Block* Block::getFather()
{
	return blockFather;
}

/**
* Discription: get block id
* @param: 
* @return: the block's id
*/
int Block::getBlockId()
{
	return blockId;
}

/**
* Discription: get block data
* @param: 
* @return: the block's data
*/
char* Block::getData()
{
	return data;
}

/**
* Discription: get the block's height 
* @param: 
* @return: 
*/
int Block::getHeight()
{
	return height;
}

/**
* Discription: calculate the block's hushed data
* @param: 
* @return: the hash signature of the data as a null-terminated char array
*/
char* Block::hashBlock()
{
	int nonce = generate_nonce(blockId, blockFather->getBlockId());
	return generate_hash(data, length, nonce);
}

/**
* Discription: set the block's father and rase the height +1 
* @param: Block* newFather
* @return: 
*/
void Block::setFather(Block* newFather)
{

	blockFather = newFather;
	height = this->getFatherHighet() + 1;
}

/**
* Discription: set the block's data
* @param: char* newData
* @return: 
*/
void Block::setData(char* newData)
{
	data = newData;
}

/**
* Discription: get the block's prune status
* @param: 
* @return: the block's prune status
*/
bool Block::isPrune()
{
	return pruneBlock;
}

/**
* Discription: set the block's prune status to false
* @param: 
* @return: 
*/
void Block::setNoPrune()
{
	pruneBlock = false;
}

/**
* Discription: set the block's toLongest status
* @param: 
* @return:
*/
void Block::setToLongest(bool val)
{
	isLongest = val;
}

/**
*Discription: get the block's toLongest status
*@param: 
*@return: the block's toLongest status
*/
bool Block::isToLongest()
{
	return isLongest;
}

/**
*Discription: get the block's father highet
*@param: 
*@return: the block's father highet
*/
int Block::getFatherHighet()
{	
	if(blockFather == NULL)
	{
		return 0;
	}
	return blockFather->getHeight();
}

/**
* block distructor
*/
Block::~Block()
{
	delete[](data);
}
