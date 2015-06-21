/*
* the cpp file for class chain
*/
#include "Chain.h"
#include <stdlib.h>
#include "Block.h"
#include "hash.h"

#define LOCK(x) if (pthread_mutex_lock(&(x))) return ERROR;
#define LOCKE(x) if (pthread_mutex_lock(&(x))) exit(FAILURE);
#define UNLOCK(x) if (pthread_mutex_unlock(&(x))) return ERROR;
#define UNLOCKE(x) if (pthread_mutex_unlock(&(x))) exit(FAILURE);
bool Chain::isInitiated = false;


int inProccessId = EMPTY;

/**
* Discription: default constructor
* @param: 
* @return:
*/
Chain::Chain()
{
	pthread_mutex_init(&pendingBlocksMutex, NULL);
	pthread_mutex_init(&longestBlocksMutex, NULL);
	pthread_mutex_init(&blocksMapMutex, NULL);
	pthread_mutex_init(&addBlockMutex, NULL);
	pthread_cond_init(&pendingBlocksCond, NULL);
	this->size = 0;
	highestId = 0;
	daemonStop = false;
	isClosed = false;
	attachNowInProcess = false;
	isInitiated = false;
}

/**
* Discription: the thread that control everything
* @param: void* gen
* @return:in
*/
void *Chain::daemonFunc(void* gen)
{
	Chain * chain = (Chain *) gen;
	Block* handleBlock;
	while(true)
	{

		LOCKE(chain->pendingBlocksMutex);

		if(chain->daemonStop)
		{
			UNLOCKE(chain->pendingBlocksMutex);
			break;
		}


		if(chain->pendingBlocks.empty() || chain->isAttachNow())
		{
			if(pthread_cond_wait(&chain->pendingBlocksCond, &chain->pendingBlocksMutex))
			{
				exit(FAILURE);
			}
			//Awaken by close.
			if(chain->pendingBlocks.empty() || chain->daemonStop)
			{
				UNLOCKE(chain->pendingBlocksMutex);
				continue;
			}
		}
		handleBlock = chain->pendingBlocks.front();
		inProccessId = handleBlock->getBlockId();
		chain->pendingBlocks.pop_front();
		UNLOCKE(chain->pendingBlocksMutex);
		//start Hashing
		if(!handleBlock->isAttached)
		{

			if(chain->addBlockHandler(handleBlock) == ERROR)
			{
				exit(FAILURE);
			}
		}
		inProccessId = EMPTY;

	}
	//Kill daemon
	char* hashedData;
	LOCKE(chain->pendingBlocksMutex);
	while(!chain->pendingBlocks.empty())
	{
		handleBlock = chain->pendingBlocks.front();
		hashedData = handleBlock->hashBlock();
		cout << hashedData  << "\n";
		chain->pendingBlocks.pop_front();
		delete[](handleBlock->data);
		handleBlock->setData(hashedData);
		delete(handleBlock);

	}
	UNLOCKE(chain->pendingBlocksMutex);
	chain->clearChain();
	delete(chain);
	return SUCCESS;
}


/**
* Discription: manager of the add block 
* @param: Block* handleBlock
* @return: 1 for success, 0 for fail
*/
int Chain::addBlockHandler(Block* handleBlock)
{
	char* hashedData;
	if(handleBlock->getFather() != NULL)
	{
		hashedData = handleBlock->hashBlock();
	}
	LOCK(longestBlocksMutex);
	if(handleBlock->isToLongest())
	{
		handleBlock->setFather(chooseRandomLongest());
	}
	//Making sure the father didn't change
	while(handleBlock->getFather() == NULL)
	{
		handleBlock->setFather(chooseRandomLongest());
		hashedData = handleBlock->hashBlock();
	}
	UNLOCK(longestBlocksMutex);
	//Replacing the data with hashedData.
	delete[](handleBlock->data);
	handleBlock->setData(hashedData);

	//Done with hash start adding
	LOCK(blocksMapMutex);
	blocksMap.insert(std::make_pair(handleBlock->getBlockId(), handleBlock));
	UNLOCK(blocksMapMutex);
	handleBlock->isAttached = true;
	increaseSize();
	//Poping handle block.
	//Update longest blocks list.
	LOCK(longestBlocksMutex);
	updateLongest(handleBlock); 
	// list and decide who need to be prune.
	UNLOCK(longestBlocksMutex);
	return SUCCESS;

}

/**
*Discription: increase the chain size 
*@param: 
*@return:
*/
void Chain::increaseSize()
{
	size++;
}

/**
*Discription: get a block by a given id 
*@param: int blockId
*@return: the block that his id were given, null if not exist
*/
Block* Chain::getBlockById(int blockId)
{
	unordered_map<int, Block*>::const_iterator got = blocksMap.find(blockId);
	if ( got == blocksMap.end() )
	{
		return NULL;
	}
	return got->second;
}

/**
*Discription: choosing the longest list randomly
*@param: 
*@return: the chosen list
*/

Block* Chain::chooseRandomLongest()
{
	int randomElement = rand() % longestBlocks.size();
	return longestBlocks.at(randomElement);
}

/**
*Discription: updating the longest list
*@param: Block* handleBlock
*@return: 
*/
void Chain::updateLongest(Block* handleBlock)
{
	if(longestBlocks.size() > 0)
	{
		if(longestBlocks.at(0)->getHeight() < handleBlock->getHeight())
		{
			longestBlocks.clear();
		}

		else if(longestBlocks.at(0)->getHeight() > handleBlock->getHeight())
		{
			return;
		}
	}
	longestBlocks.push_back(handleBlock);
}	

/**
* Discription: get the next available id for an added block
* @param: 
* @return: the new id block
*/
int Chain::getNextBlockId()
{

	if(blockIds.empty())
	{
		highestId++;

		return highestId;
	}
	int returnValue = blockIds.top();
	blockIds.pop();	
	return returnValue;
}

/**
* Discription: get the chain size
* @param: 
* @return: chain size
*/
int Chain::getSize()
{
	if(!isInitiated)
	{
		return ERROR;
	}
	return size;
}

/**
* Discription: gets block id, search the block in pendingBlocks and return it
* @param: int blockId
* @return: the needed block
*/
Block* Chain::foundInPending(int blockId)
{ 
	int i = 0;
	int dequeSize = pendingBlocks.size();
	while(i < dequeSize) 
	{ 
		if (pendingBlocks.at(i)->getBlockId() == blockId)
		{ 
			return pendingBlocks.at(i);
		}
		i++;
	}
	return NULL;
}

/**
*DESCRIPTION: This function initiates the Block chain, and creates the genesis Block. 
*The genesis Block does not hold any transaction data   
*or hash. This function should be called prior to any other functions as a necessary
*precondition for their success (all other functions should   
*return with an error otherwise).
*RETURN VALUE: On success 0, otherwise -1.
*/
int Chain::init()
{
	if(isInitiated)
	{
		return ERROR;
	}
	Block* newRootBlock = new Block(0, NULL , 0, NULL);
	newRootBlock->setNoPrune();
	this->longestBlocks.push_back(newRootBlock);
	this->blocksMap.insert(std::make_pair(newRootBlock->getBlockId(), newRootBlock));
	init_hash_generator();
	isInitiated = true;
	pthread_create(&daemonThread , NULL, &Chain::daemonFunc, this);
	return SUCCESS;
}

/**
*DESCRIPTION: Ultimately, the function adds the hash of the data to the Block chain.
*Since this is a non-blocking package, your implemented method should return as soon
*as possible, even before the Block was actually attached to the chain.
*Furthermore, the father Block should be determined before this function returns. 
*The father Block should be the last Block of the 
*current longest chain (arbitrary longest chain if there is more than one).
*Notice that once this call returns, the original data may be freed by the caller.
*RETURN VALUE: On success, the function returns the lowest available block_num (> 0),
*which is assigned from now on to this individual piece of data.
*On failure, -1 will be returned.
*/
int Chain::addBlock(char *data , int length)
{
	if(daemonStop || !isInitiated)
	{
		return ERROR;
	}
	int newId =  getNextBlockId();
	Block* newBlock = new Block(newId, data, length, chooseRandomLongest());
	LOCK(pendingBlocksMutex);
	pendingBlocks.push_back(newBlock);
	UNLOCK(pendingBlocksMutex);
	if(pthread_cond_signal(&pendingBlocksCond))
	{
		return ERROR;
	}
	return newId;
}

/**
*DESCRIPTION: Without blocking, enforce the policy that this block_num should be attached 
*to the longest chain at the time of attachment of the Block. For clearance, this is opposed
*to the original add_block that adds the Block to the longest chain during the time that 
*add_block was called.
*The block_num is the assigned value that was previously returned by add_block. 
*RETURN VALUE: If block_num doesn't exist, return -2; In case of other errors, return -1;
*In case of success return 0; In case block_num is already attached return 1.
*/
int Chain::toLongest(int block_num)
{
	if(daemonStop || !isInitiated)
	{
		return ERROR;
	}
	Block* pendingBlock;
	LOCK(blocksMapMutex);
	unordered_map<int, Block*>::const_iterator got = blocksMap.find(block_num);
	if( got != blocksMap.end() || inProccessId == block_num)
	{
		UNLOCK(blocksMapMutex);	
		return ALREADY_ADDED;
	}
	UNLOCK(blocksMapMutex);
	LOCK(pendingBlocksMutex);
	if ((pendingBlock = foundInPending(block_num)) != NULL)
	{
		pendingBlock->setToLongest(true);
		UNLOCK(pendingBlocksMutex);
		return SUCCESS;
	}
	UNLOCK(pendingBlocksMutex);
	return FAIL;
}

/**
*DESCRIPTION: This function blocks all other Block attachments, 
*until block_num is added to the chain. The block_num is the assigned value 
*that was previously returned by add_block.
*RETURN VALUE: If block_num doesn't exist, return -2;
*In case of other errors, return -1; In case of success or if it is already 
*attached return 0.
*/
int Chain::attachNow(int block_num)
{
	if(daemonStop || !isInitiated)
	{
		return ERROR;
	}
	attachNowInProcess = true;
	Block* attachNow;
	//Already attached.
	LOCK(blocksMapMutex);
	LOCK(pendingBlocksMutex);
	LOCK(longestBlocksMutex);
	if(getBlockById(block_num) != NULL || inProccessId == block_num)
	{
		attachNowInProcess = false;
		UNLOCK(pendingBlocksMutex);
		UNLOCK(longestBlocksMutex);
		UNLOCK(blocksMapMutex);
		if(pthread_cond_signal(&pendingBlocksCond))
		{
			return ERROR;
		}
		return  SUCCESS;
	}
	//Inside the pending least.
	if((attachNow = foundInPending(block_num)) != NULL)
	{		
		pendingBlocks.erase(std::remove(pendingBlocks.begin(), \
			pendingBlocks.end(), attachNow), pendingBlocks.end());
		UNLOCK(pendingBlocksMutex);
		UNLOCK(longestBlocksMutex);
		UNLOCK(blocksMapMutex);
		int returnValue = addBlockHandler(attachNow);
		if(returnValue)
		{
			return ERROR;
		}
		attachNowInProcess = false;
		if(pthread_cond_signal(&pendingBlocksCond))
		{
			return ERROR;
		}
		return returnValue;
	}
	attachNowInProcess = false;
	UNLOCK(pendingBlocksMutex);
	UNLOCK(longestBlocksMutex);
	UNLOCK(blocksMapMutex);
	if(pthread_cond_signal(&pendingBlocksCond))
	{
		return ERROR;
	}
	return FAIL;
}

/**
*DESCRIPTION: Without blocking, check whether block_num was added to the chain.
*The block_num is the assigned value that was previously returned by add_block.
*RETURN VALUE: 1 if true and 0 if false. If the block_num doesn't exist, return -2;
*In case of other errors, return -1.
*/
int Chain::wasAdded(int block_num)
{
	if(!isInitiated)
	{
		return ERROR;
	}
	LOCK(blocksMapMutex);
	std::unordered_map<int, Block*>::const_iterator got = blocksMap.find(block_num);
	//doesn't exist at all
	if(got != blocksMap.end())
	{
		UNLOCK(blocksMapMutex);
	    return ALREADY_ADDED;
	}
	else if (foundInPending(block_num) != NULL || inProccessId == block_num)
	{
		UNLOCK(blocksMapMutex);
		return IN_PENDING;
	}
	else
	{
		UNLOCK(blocksMapMutex);
		return DOESNT_EXIST;
	}
}

/**
*DESCRIPTION: delete from the block map.
*RETURN VALUE:
*/
void Chain::deleteFromMap(int deleteId)
{
	blocksMap.erase(deleteId);
}

/**
*DESCRIPTION: delete all the prune blocks from the map
*RETURN VALUE:
*/
void Chain::pruneTree()
{
	
	Block* pruneBlock;
	for (std::unordered_map<int, Block* >::iterator it = blocksMap.begin();\
	 it != blocksMap.end(); )
	{
		
		if (it->second->isPrune() == true)
		{

			pruneBlock = it->second;
			int id = pruneBlock->getBlockId();
			blockIds.push(id);
			blocksMap.erase(it++);
			delete(pruneBlock);
		}
		else
		{
			++it;
		}
	}

}

/**
*DESCRIPTION: delete all the prune blocks from the map
*RETURN VALUE:
*/
void Chain::tagPrune(Block* tagBlock)
{
	while(tagBlock->getFather() != NULL && tagBlock->isPrune() == true)
	{

		tagBlock->setNoPrune();
		//Next block
		tagBlock = tagBlock->getFather();
	}
}

/**
*DESCRIPTION: Search throughout the tree for sub-chains that are not the longest chain,
*detach them from the tree, free the blocks, and reuse the block_nums.
*RETURN VALUE: On success 0, otherwise -1.
*/
int Chain::pruneChain()
{
	if(daemonStop || !isInitiated)
	{
		return ERROR;
	}
	LOCK(pendingBlocksMutex);
	LOCK(blocksMapMutex);
	//choosing random father
	Block* chosenLeaf = chooseRandomLongest();
	LOCK(longestBlocksMutex);
	tagPrune(chosenLeaf);
	pruneTree();
	longestBlocks.clear();
	longestBlocks.push_back(chosenLeaf);
	UNLOCK(pendingBlocksMutex);
	UNLOCK(blocksMapMutex);
	UNLOCK(longestBlocksMutex);
	return SUCCESS;
}

/**
*DESCRIPTION: Close the recent blockchain and reset the system, so that it is possible
*to call init_blockchain again. Non-blocking. All pending Blocks should be hashed and
*printed to terminal (stdout). Calls to library methods which try to alter the state of
*the Blockchain are prohibited while closing the Blockchain. e.g.: Calling chain_size() is ok,
*a call to prune_chain() should fail.
*In case of a system error, the function should cause the process to exit. 
*/
void Chain::closeChain()
{
	if (daemonStop)
	{
		return;
	}
	daemonStop = true;
	pthread_cond_signal(&pendingBlocksCond);

}

/**
*DESCRIPTION: The function blocks and waits for close_chain to finish.
*RETURN VALUE: If closing was successful, it returns 0.
*If close_chain was not called it should return -2. In case of other error,
*it should return -1
*/
int Chain::returnOnClose()
{
	if(!isInitiated)
	{
		return ERROR;
	}

	if (isClosed)
	{
		return ALREADY_CLOSED;
	}
	if (daemonStop)
	{
		//Waiting for daemon to finish
		return pthread_join(daemonThread, NULL);
	}
	//called before close itself
	return CALLED_BEFORE_CLOSE_CHAIN;
}



/**
*DESCRIPTION: closing and deleting all the block chain
*RETURN VALUE:
*/
void Chain::clearChain()
{
	close_hash_generator();
	for (unordered_map<int, Block *>::iterator it = blocksMap.begin();\
	 it != blocksMap.end(); )
	{
		delete (it->second);
		it = blocksMap.erase(it);
	}
	while(blockIds.empty())
	{
		blockIds.pop();
	}
	longestBlocks.clear();
	size = 0;
	isInitiated = false;
	isClosed = true;


}

/**
*DESCRIPTION: return the status of attach now of the block
*RETURN VALUE: true if attach, false otherwise
*/
bool Chain::isAttachNow()
{
	return attachNowInProcess;
}

/**
*Chain distractor
*/
Chain::~Chain()
{
	pthread_mutex_destroy(&pendingBlocksMutex);
	pthread_mutex_destroy(&longestBlocksMutex);
	pthread_mutex_destroy(&blocksMapMutex);
	pthread_mutex_destroy(&addBlockMutex);
	pthread_cond_destroy(&pendingBlocksCond);
}


