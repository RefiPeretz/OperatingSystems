/**
* the header file  for the class chain
*/
#ifndef _CHAIN_H
#define _CHAIN_H
#include <deque>
#include <queue>     
#include <unordered_map>
#include "Block.h"
#include <algorithm>
#include <mutex>
#include <utility>
#include <chrono>
#include <functional>
#include <atomic>

// global difinitions.
#define NON_TAKEN 0
#define TAKEN 1
#define ALREADY_ADDED 1
#define IN_PENDING 0
#define DOESNT_EXIST -2
#define ALREADY_CLOSED 1
#define CALLED_BEFORE_CLOSE_CHAIN -2
#define FAILURE 1

class Chain
{
public:
	Chain();
	pthread_mutex_t pendingBlocksMutex;
	pthread_mutex_t longestBlocksMutex;
	pthread_mutex_t blocksMapMutex;
	pthread_mutex_t addBlockMutex;
	pthread_cond_t pendingBlocksCond;
	pthread_t daemonThread;
	/**
	* Discription: set array of block's id
	*/
	std::priority_queue<int, std::vector<int>, std::greater<int> > blockIds;
	/**
	* Discription: get the untaken id to a new block
	*/
	int getNextBlockId();
	/**
	* Discription: increase Size of block by 1
	*/
	void increaseSize();
	/**
	* Discription: choos the longest random chain
	*/
	Block* chooseRandomLongest();
	/**
	* Discription: set all the block of the longest chain prune value to true 
	*/
	void tagPrune(Block* chosenLeaf);
	/**
	* Discription: get a block by a given id
	*/
	Block* getBlockById(int block_num);
	/**
	* Discription: get number of block that were added since init
	*/
	int getSize();
	std::deque<Block*> pendingBlocks;
	std::vector<Block*> longestBlocks;
	std::unordered_map<int, Block*> blocksMap;
	/**
	* Discription: the lead thread that control everything
	*/
	static void * daemonFunc(void* gen);
	/**
	* Discription: This function initiates the Block chain
	*/
	int init();
	/**
	* Discription: add new block
	*/
	int addBlock(char *data , int length);
	/**
	* Discription: enforce the policy that this block_num should be attached
	* to the longest chain at the time of attachment of the Block
	*/
	int toLongest(int block_num);
	/**
	* Discription: function blocks all other Block attachments, until block_num
	* is added to the chain.
	*/
	int attachNow(int block_num);

	/**
	* Discription: check whether block_num was added to the chain
	*/
	int wasAdded(int block_num);
	/**
	* Discription: Search throughout the tree for sub-chains that are not the longest
	* chain, detach them from the tree, free the blocks, and reuse the block_nums.
	*/
	int pruneChain();
	/**
	* Discription: iterate all the map and delete blocks that are for prune
	*/
	void pruneTree();
	/**
	* Discription: Close the recent blockchain and reset the system
	*/
	void closeChain();
	/**
	* Discription: blocks and waits for close_chain to finish
	*/
	int returnOnClose();
	/**
	* Discription: clear a list that is not the longest
	*/
	void updateLongest(Block* handleBlock);
	/**
	* Discription: check if a block is in the panding list
	*/
	Block* foundInPending(int id);
	/**
	* Discription: delete a block from the vector
	*/
	void deleteFromVector(int deleteId);
	/**
	* Discription: delete a block from longest
	*/
	void deleteFromLongest(int blockId);
	/**
	* Discription: delete a block from map
	*/
	void deleteFromMap(int deleteId);
	/**
	* Discription: handeler of the added blocks
	*/
	int addBlockHandler(Block* handleBlock);
	/**
	* Discription: delete the block chain
	*/
	void clearChain();
	/**
	* Discription: a function that check the status of a block if it need to
	* be attached now
	*/
	bool isAttachNow();
	/**
	* Discription: Chain distractor
	*/
	~Chain();



	static bool isInitiated;
private:
	bool isClosed;
	int highestId;
	bool attachNowInProcess;
	bool daemonStop;
	int size;
	int highet;
	Block* rootBlock;
	Block* currentLongest;

};

#endif
