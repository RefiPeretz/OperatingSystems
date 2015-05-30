/**
* CacheManager.cpp
* Date: 21/5/2015
*/
#include "CacheManager.h"


struct My
{
	static bool blockComparator    (Block*    first, Block*    second);

};

bool My::blockComparator(Block*    first, Block*    second) 
{ 
	return first->getCounter() > second->getCounter(); 
}



/**
* Discription: defulte constructor
* @param:
* @return: nothing
*/
CacheManager::CacheManager(char* root, int numberOfBlock, int blockSizeStandard)
{
	_rootDir = root;
	_cacheSize = numberOfBlock;
	_blockSize = blockSizeStandard;
	std::fstream filesystem;
	strcat(_fsPath, _rootDir);
	strcat(_fsPath, "/.filesystem.log"); //TODO . add the dot back.
	cout << _fsPath << endl;
	_fs.open(_fsPath, std::fstream::out | std::fstream::app);
	

}

/**
* Discription: get function
* @param: 
* @return: block standart size
*/
int CacheManager::getStandartdSize()
{
	return _blockSize;
}

/**
* Discription: get function
* @param: 
* @return: root directory
*/
char* CacheManager::getRootDir()
{
	return _rootDir;
}



/**
* Discription: write to the log function
* @param: std::string func
* @return: 
*/
void CacheManager::writeToLog(std::string func)
{

	time_t result = time(nullptr); //TODO may be free?

	_fs << result << " " << func << endl;

}


/**
* Discription: searching for a block in the cache. if not found return null 
* @param: std::string name, int blockNumber
* @return: the block that was found
*/
Block* CacheManager::findMyBlock(string name, int blockNumber)
{
	// try to fine the block in the chach, with it number
	for (std::list<Block*>::iterator it = blockList.begin(); it != blockList.end();
			++it) 
	{
		if (((*it)->getBlockFileName()).compare(name) == 0 && blockNumber == (*it)->\
			getBlockPosition()) 
		{
			return (*it);
		}
	}
	return NULL;
}

/**
* Discription: adding the new block to the cache
* @param: int fd, const char* path, int positionBlock, Block*& newBlock
* @return: result of the adding
*/
int CacheManager::addBlockToCache(int fd, const char* path, int positionBlock, Block*& newBlock)
{
	makeRoomInCache(); //TODO
	char * blockData = new char[_blockSize + 1];
	int preadResult = pread(fd, blockData, _blockSize, _blockSize*positionBlock);
	if(preadResult < 0)
	{
		delete[](blockData);
		return preadResult; //TODO check error
	}
	char * pathCopy = new char[strlen(path) + 1];
	strcpy(pathCopy, path);
	newBlock = new Block(pathCopy, _blockSize*positionBlock, blockData, positionBlock);
	blockList.push_back(newBlock);
	blockList.sort(My::blockComparator); //TODO check sort order.
	delete[](pathCopy);
	return preadResult; //TODO check error

}

/**
* Discription: loads blocks into the cache.
* @param: int fd, const char* filePath, off_t offset, size_t readSize, char* buf
* @return: the bytes that we read
*/
int CacheManager::cacheRead(int fd, const char* filePath, off_t offset, size_t readSize, char* buf, struct fuse_file_info *fi)
{
	
	//cout << "size of file is: " << readSize << endl; //------------------
	//fail if file handle is closed
	//if (fcntl(fi->fh, F_GETFL) < 0 && errno == EBADF) // ------------- TODO important
	//{
	//	return -EBADF;
	//}
	int bytesRead = 0;
	size_t startBlockIndex = floor( offset / _blockSize);
	size_t startBlockOffset = offset % _blockSize;

	size_t endBlockIndex = floor( (offset + readSize) / _blockSize);
	size_t endBlockOffset = (offset + readSize) % _blockSize;
	if(endBlockOffset == 0)
	{
		endBlockIndex--;
		endBlockOffset = _blockSize;
	}
	Block* newBlock;

	for (size_t i = startBlockIndex; i <= endBlockIndex; i++)
	{
		//block was not found in the chach - write to cache
		if ((newBlock = findMyBlock(filePath, i)) == NULL) //TODO are we sure we use i
		{
			
			int resAddBlock = addBlockToCache(fd, filePath, i, newBlock);
			if (resAddBlock < 0)
			{
				return resAddBlock; //TODO error
			}
		}

		cout << "iteration :" << i << endl;
		cout << "now running block: " << newBlock->getBlockPosition() << endl;
		//newBlock->setBlockPosition(i); 

		newBlock->addToCounter(); // for ioctl
				// Block exist
		if (startBlockIndex == endBlockIndex)
		{
			// Reading from one block only
			strncpy(buf + bytesRead, (newBlock->getBlockData()) + startBlockOffset, readSize);
			bytesRead += readSize;
			//cout<<"writing in equal cond: "<<newBlock->getBlockData()<<endl; 
			break;
		}

		// Reading from multiple blocks
		if (i == startBlockIndex)
		{
			// First block
			strncpy(buf + bytesRead, newBlock->getBlockData() + startBlockOffset,\
			 _blockSize - startBlockOffset);
			cout << "Writing to first" << newBlock->getBlockData() << endl;
			bytesRead += _blockSize - startBlockOffset;
		}
		else if (i == endBlockIndex)
		{

			strncpy(buf + bytesRead, newBlock->getBlockData(), endBlockOffset);
			//cout<<"Writing to second"<<newBlock->getBlockData()<<endl;
			bytesRead += endBlockOffset;
		}
		else
		{
			// Middle blocks
			strncpy(buf + bytesRead, newBlock->getBlockData(), _blockSize);
			//cout<<"Writing to second"<<newBlock->getBlockData()<<endl;
			bytesRead += _blockSize;
		}
		
	}//TODO get to end of file. what we do
	return bytesRead;
}

/**
* Discription: makeing room in the cache
* @param:
* @return: 
*/

void CacheManager::makeRoomInCache()
{
	cout << "trying to make room" << endl;
	// list smaller then the csche size
	if (blockList.size() < _cacheSize)
	{
		return;
	}
	else
	{
		Block* tempBlock = blockList.back();
		blockList.pop_back();
		delete(tempBlock);
	}
}

/**
* Discription: rename the given file/directory 
* @param: const char *path, const char *newpath, bool folder
* @return: 
*/


void CacheManager::Rename(std::string oldName , std::string newName)
{
	for (std::list<Block*>::iterator it = blockList.begin(); it != blockList.end();
		++it)
	{
		if ((*it)->getBlockFileName().length() >= oldName.length())
		{
			std::string tmpName = (*it)->getBlockFileName().substr(0,oldName.length());
			if (tmpName == oldName)
			{
				std::string update = (*it)->getBlockFileName().replace(0,oldName.length(),newName);
				(*it)->setBlockFileName(update);
			}
		}
	}

}




std::string CacheManager::toString()
{
    std::stringstream ss;
    ss<< endl;

    for (std::list<Block*>::iterator it = blockList.begin(); it != blockList.end();
			++it)
    {
    	ss<<(*it)->toString(_rootDir);
    }
    _fs<<ss.str()<<endl;
    return ss.str();

}


/**
* Discription: distractor of cache manager
* @param: 
* @return: 
*/

CacheManager::~CacheManager()
{
	_fs.close();
	Block* temp;
	for (std::list<Block*>::iterator it = blockList.begin(); it != blockList.end();
		++it)
	{
		temp = (*it);
		delete(temp);
	}
	blockList.clear();
	temp = NULL;


}