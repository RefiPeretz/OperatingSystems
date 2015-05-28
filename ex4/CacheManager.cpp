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
CacheManager::CacheManager(char* root, char* mount, int numberOfBlock, int blockSizeStandard)
{
	_rootDir = root;
	_cacheSize = numberOfBlock;
	_blockSize = blockSizeStandard;
	std::fstream filesystem;
	cout << "before open" << endl;
	strcat(_fsPath, _rootDir);
	cout << "try to open" << endl;
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
* Discription: distractor of cache manager
* @param: 
* @return: 
*/
CacheManager::~CacheManager()
{
	_fs.close();
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
	//blockData[0] = '\0';
	int preadResult = pread(fd, blockData, _blockSize, _blockSize*positionBlock);
	if(preadResult < 0)
	{
		return preadResult; //TODO check error
	}
	char * pathCopy = new char[strlen(path) + 1];
	strcpy(pathCopy, path);
	newBlock = new Block(pathCopy, _blockSize*positionBlock, blockData, positionBlock);
	blockList.push_back(newBlock);
	blockList.sort(My::blockComparator); //TODO check sort order.
	return preadResult; //TODO check error

}

/**
* Discription: loads blocks into the cache.
* @param: int fd, const char* filePath, off_t offset, size_t readSize, char* buf
* @return: the bytes that we read
*/
int CacheManager::cacheRead(int fd, const char* filePath, off_t offset, size_t readSize, char* buf)
{
	
	cout << "size of file is: " << readSize << endl;

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
	cout << "ofsset value is: " << offset << endl;
	cout << "start from block number: " << startBlockIndex << endl;
	cout << "end in block number: " << endBlockIndex << endl;
	cout << "Running from ofsset: " << startBlockOffset << " to : " << endBlockOffset << endl;
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

	//buf[bytesRead] = '\0';
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

// int CacheManager::Rename(std::string path, std::string newpath, bool folder)
// {	
// 	// char * pathCopy;
// 	// std::string cutPath;
// 	// const char* addPath;


// 	// int pathDelta = newpath.length() - path.length();
// 	// 	// newFolderPath = new char[oldPathLen + 2];
// 	// 	// strcpy(newFolderPath, path);
// 	// 	// strcat(newFolderPath, "/");  //TODO magic number



// 	// for (std::list<Block*>::iterator it = blockList.begin(); it != blockList.end();
// 	// 		++it) 
// 	// {
// 	// 	if(folder && ((string)(*it)->getBlockFileName()).find(path) == 0)
// 	// 	{
// 	// 		pathCopy = new char[pathDelta + 1 + strlen((*it)->getBlockFileName())];
// 	// 		cutPath = ((std::string)(*it)->getBlockFileName()).substr(path.length());
// 	// 		addPath = cutPath.c_str();
// 	// 		strcpy(pathCopy,addPath);
// 	// 		delete((*it)->getBlockFileName());
// 	// 		cout<<"after change in folder the path is: "<<pathCopy<<endl;
// 	// 		(*it)->setBlockFileName(pathCopy);


// 	// 	}
// 	// 	else if(((std::string)(*it)->getBlockFileName()).compare(path) != 0)
// 	// 	{
// 	// 		pathCopy = new char[path.length() + pathDelta];
// 	// 		strcpy(pathCopy, newpath.c_str());
// 	// 		delete((*it)->getBlockFileName());
// 	// 		cout<<"after change in only file the path is: "<<pathCopy<<endl;
// 	// 		(*it)->setBlockFileName(pathCopy);

// 	// 	}

// 	//  }
	

// 	// if (folder)
// 	// {
// 	// 	oldPathLen = strlen(path);
// 	// 	pathLenDiff = strlen(newpath) - oldPathLen;
// 	// 	folderPath = new char[oldPathLen + 2];  // +2 to account for / and \0
// 	// 	strcpy(folderPath, path);
// 	// 	strcat(folderPath, FOLDER_SEPARATOR);  // add "/" to end of path
// 	// }

// 	// // goes over the map, and finds blocks that should be renamed
// 	// for (std::list<Block*>::iterator it = blockList.begin(); it != blockList.end();
// 	// 		++it)
// 	// {
// 	// 	if (folder && checkPrefix(folderPath, (*it)->getBlockFileName()))  // renaming a folder
// 	// 	{
// 	// 		pathCopy = new char[strlen(mapIt->second->_path) + pathLenDiff + 1];
// 	// 		strcpy(pathCopy, newpath);
// 	// 		strcat(pathCopy, mapIt->second->_path + oldPathLen);  // -1 to include "/"
// 	// 		delete mapIt->second->_path;
// 	// 		mapIt->second->_path = pathCopy;
// 	// 		blocksToReadd.push_back(mapIt->second);
// 	// 		mapIt = _blocksByKey.erase(mapIt);
// 	// 	}
// 	// 	else if (strcmp(mapIt->second->_path, path) == 0)  // renaming a single file
// 	// 	{
// 	// 		delete mapIt->second->_path;
// 	// 		pathCopy = new char[strlen(newpath) + 1];
// 	// 		mapIt->second->_path = strcpy(pathCopy, newpath);
// 	// 		blocksToReadd.push_back(mapIt->second);
// 	// 		mapIt = _blocksByKey.erase(mapIt);
// 	// 	}
// 	// 	else
// 	// 	{
// 	// 		++mapIt;
// 	// 	}
// 	// }

// 	// // readd the renamed blocks
// 	// vector<Block*>::iterator vecIt;
// 	// for (vecIt = blocksToReadd.begin(); vecIt != blocksToReadd.end(); ++vecIt)
// 	// {
// 	// 	_blocksByKey[_generateKey((*vecIt)->_path, (*vecIt)->_blockIndexInFile)] = *vecIt;
// 	// }

// 	// if (isFolder)
// 	// {
// 	// 	delete folderPath;
// 	// }
// 	return 0;

// }