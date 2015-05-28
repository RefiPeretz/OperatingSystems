#include <sys/time.h>
#include <stdlib.h>
#include <string>
#include <algorithm>
#include <vector>
#include <iostream>
#include <sstream>
#define SUCCESS 0
#define FAILURE -1
/**
 * Class Block, represent a block in the Cache
 */
class Block
{
public:
	std::string fileName;
	int position;
	struct timeval *access;
	char* mem;
	bool used;
	int sizeBlock;
	void rename(std::string newName);
	Block(int sizeOfblock);
	std::string toString();
	void setAccess();
	void initBlock(void* buffer, std::string fileName, int position);
	~Block();
};
/**
 * Block constructor
 */
Block::Block(int sizeOfBlock)
{
	sizeBlock = sizeOfBlock;
	fileName = "/0";
	position = -1;
	used = false;
	access = NULL;
	mem = (char *)malloc(sizeOfBlock);
	if (mem == NULL)
	{
		exit(FAILURE);
	}

}
/**
 * Block destructor
 */
Block::~Block()
{
	free(mem);
	free(access);

}
/**
 * sets the access time
 */
void Block::setAccess()
{
	if (access != NULL)
	{
		free(access);
	}
	//sets the access
	access = (struct timeval*)malloc(sizeof(struct timeval));
	gettimeofday(access, NULL);
}
/**
 * initialize the block
 */
void Block::initBlock(void* buffer, std::string nameOfFile, int pos)
{
	if (mem != NULL)
	{
		free(mem);
	}
	mem = (char *)malloc(sizeBlock);
	memcpy(mem, buffer, sizeBlock);
	fileName = nameOfFile;
	position = pos;
	used = true;
	setAccess();
}
/**
 * returns a block representation
 */
std::string Block::toString()
{
	std::stringstream out;
	out << fileName;
	out << "\t";
	out << (position / sizeBlock) + 1;
	out << "\t";
	struct tm *nowtime = NULL;
	nowtime = localtime(&access->tv_sec);
	if (nowtime == NULL)
	{
		std::cerr << "NULL" << std::endl;
	}
	out << (nowtime->tm_mon + 1) << ":";
	out << (nowtime->tm_mday) << ":";
	out << (nowtime->tm_hour) << ":";
	out << (nowtime->tm_min) << ":";
	out << nowtime->tm_sec << ":";
	out << (access->tv_usec / 1000) << "\n";
	return out.str();
}
/**
 * changes the block's name into the given name
 */
void Block::rename(std::string newName)
{
	fileName = newName;
}
/**
 * Class cache, represents a Cache
 */
class Cache
{
	std::vector<Block*> blocks;
	int numBlocks;
	int sizeBlock;
	int writeToCache(int fd, std::string filePath, int positionBlock);
	Block* LRU();
	Block* findBlock(std::string pathName, int position);
public:
	Cache(int numOfBlocks, int sizeOfBlock);
	int readToCache(int fd, std::string filePath, off_t offset, size_t sizeToRead);
	int writeToBuffer(size_t size, off_t offset, char* buffer, std::string pathName);
	void rename(std::string oldName, std::string newName);
	std::string toString();
	~Cache();
};
/**
 * Cache constructor
 */
Cache::Cache(int numOfBlocks, int sizeOfBlock) :
	blocks(numOfBlocks)
{
	for (int i = 0; i < numOfBlocks; i++)
	{
		blocks[i] = new Block(sizeOfBlock);
	}
	numBlocks = numOfBlocks;
	sizeBlock = sizeOfBlock;
}
/**
 * Cache destructor
 */
Cache::~Cache()
{
	for (int i = 0; i < numBlocks; i++)
	{
		delete (blocks[i]);
	}
}

/**
 * loads the blocks into the cache returns 0 on SUCCESS and -1 on FAILURE
 */
int Cache::readToCache(int fd, std::string filePath, off_t offset, size_t sizeToRead)
{

	int start = offset - (offset % sizeToRead); //normalize to the start of the block
	for (unsigned int i = start; i < sizeToRead + offset; i += sizeBlock)
	{
		//if the block isnt in the cache then write it
		if (findBlock(filePath, i) == NULL)
		{
			int res = writeToCache(fd, filePath, i);
			if (res < 0)
			{
				return FAILURE;
			}
		}
	}
	return SUCCESS;

}
/**
 * writes the block into the cache,
 * returns 0 on success and -1 on failure
 */
int Cache::writeToCache(int fd, std::string filePath, int positionBlock)
{
	Block* avBlock = NULL;
	for (unsigned int i = 0; i < blocks.size(); i++)
	{
		if (!blocks[i]->used)
		{
			avBlock = blocks[i];
			//			perror("found av block");
			break;
		}
	}
	//if couldnt find available block
	if (avBlock == NULL)
	{
		avBlock = LRU();
	}
	if (avBlock == NULL)
	{
		return FAILURE;
	}
	void *buff;
	int check = posix_memalign((void**)&buff, sizeBlock, sizeBlock);
	if (check != 0)
	{
		buff = malloc(sizeBlock);
	}
	if (buff == NULL)
	{
		return FAILURE;
	}
	memset(buff, '\0', sizeBlock);
	//reads into the buffer
	int read = pread(fd, buff, sizeBlock, positionBlock);
	if (read < 0)
	{
		return FAILURE;
	}
	if (read == 0)
	{
		return SUCCESS;
	}
	//sets the block
	avBlock->initBlock(buff, filePath, positionBlock);
	free(buff);
	return SUCCESS;
}
/**
 * returns the least recently used block
 */
Block* Cache::LRU()
{
	int i;
	Block *min;
	for (i = 0; i < numBlocks; i++)
	{
		//finds the min block
		if (blocks[i]->used)
		{
			min = blocks[i];
			break;
		}
	}
	for (int j = i + 1; j < numBlocks; j++)
	{
		if (blocks[j]->used)
		{
			//checks if there is other min block
			bool res = timercmp(blocks[j]->access , min->access,<);
			if (res)
			{
				min = blocks[j];
			}
		}
	}
	return min;
}
/**
 * loads the blocks in the cache into the given buffer
 * returns tne number of bytes that was written to the buffer. and -1 on FAILURE
 */
int Cache::writeToBuffer(size_t size, off_t offset, char* buffer, std::string pathName)
{
	int counter = 0;
	unsigned int start = offset - (offset % sizeBlock); //normalize to the start of the block
	unsigned int end = ((size + offset) / sizeBlock) * sizeBlock; // normalize to the end block.
	unsigned int myOffset = offset % sizeBlock;
	for (unsigned int i = start; i < size + offset; i += sizeBlock)
	{
		Block* b = findBlock(pathName, i);
		if (b == NULL)
		{
			break;
		}
		//first block
		if (i == start)
		{
			if (start == end)
			{
				if (size < sizeBlock - myOffset)
				{
					memcpy(buffer, b->mem + myOffset, size);
					counter += size;
					break;
				}
				else
				{
					memcpy(buffer, b->mem + myOffset, sizeBlock - myOffset);
					counter += sizeBlock - myOffset;
					break;
				}
			}
			memcpy(buffer, b->mem + myOffset, sizeBlock - myOffset);
			counter += sizeBlock - myOffset;
		}
		//last block
		else if (i == end)
		{
			memcpy(buffer + counter, b->mem, size + offset - end);
			counter += size + offset - end;
			break;
		}
		else
		{
			memcpy(buffer + counter, b->mem, sizeBlock);
			counter += sizeBlock;
		}
	}
	return counter;

}
/**
 * Block comparator
 */
bool comparator(const Block* b1, const Block* b2)
{
	//if the blocks are null
	if (b1 == NULL)
	{
		return true;
	}
	if (b2 == NULL)
	{
		return false;
	}
	if (!b1->used)
	{
		return true;
	}
	if (!b2->used)
	{
		return false;
	}
	bool res = timercmp(b1->access , b2->access,>);
	return res;
}
/**
 * Cache representation
 */
std::string Cache::toString()
{
	std::string out = "";
	std::sort(blocks.begin(), blocks.end(), comparator);
	for (unsigned int i = 0; i < blocks.size(); i++)
	{
		if (blocks[i]->used)
		{
			out += blocks[i]->toString();
		}
	}
	return out;
}
/**
 * finds the given block, if found, returns the required block, else return null
 */
Block* Cache::findBlock(std::string pathName, int position)
{
	for (unsigned int i = 0; i < blocks.size(); i++)
	{
		if (blocks[i]->used)
		{
			if (pathName.compare(blocks[i]->fileName) == 0 && position == blocks[i]->position)
			{
				return blocks[i];
			}
		}
	}
	return NULL;
}
/**
 * rename the file's name in the cache
 */
void Cache::rename(std::string oldName, std::string newName)
{
	for (int i = 0; i < numBlocks * sizeBlock; i += sizeBlock)
	{
		//finds the block
		Block* b = findBlock(oldName, i);
		if (b == NULL)
		{
			continue;
		}
		b->rename(newName);
	}
}
