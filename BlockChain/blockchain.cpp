/**
* cpp file that contains functions of blockchain provided by OS staff
*/
#include "Chain.h"
#include <unistd.h>
#include <iostream>
#include "blockchain.h"
#include "hash.h"
#include <stack>

#define ERROR_BLOCKCHAIN -1
using namespace std;
// global defintion


Chain* blockChain;

/**
 * DESCRIPTION: This function initiates the Block chain, and creates the genesis Block.  
 * The genesis Block does not hold any transaction data or hash.
 * This function should be called prior to any other functions as a necessary precondition for
 * their success (all other functions should return with an error otherwise).
 * RETURN VALUE: On success 0, otherwise -1.
 */
int init_blockchain()
{

	if(blockChain->isInitiated)
	{
		return ERROR_BLOCKCHAIN;
	}
	blockChain = new Chain();
	return blockChain->init();	
}

/**
* DESCRIPTION: Ultimately, the function adds the hash of the data to the Block chain.
* Since this is a non-blocking package, your implemented method should return as soon as 
* possible, even before the Block was actually attached to the chain.
* Furthermore, the father Block should be determined before this function returns. The father
* Block should be the last Block of the current longest chain (arbitrary longest chain if there
* is more than one).
* Notice that once this call returns, the original data may be freed by the caller.
* RETURN VALUE: On success, the function returns the lowest available block_num (> 0),
* which is assigned from now on to this individual piece of data.
* On failure, -1 will be returned.
*/
int add_block(char *data , int length)
{

	return blockChain->addBlock(data , length);
}

/**
* DESCRIPTION: Without blocking, enforce the policy that this block_num should be attached to the
* longest chain at the time of attachment of the Block. For clearance, this is opposed to the
* original add_block that adds the Block to the longest chain during the time that add_block 
* was called. The block_num is the assigned value that was previously returned by add_block. 
* RETURN VALUE: If block_num doesn't exist, return -2; In case of other errors, return -1; 
* In case of success return 0; In case block_num is already attached return 1.
*/
int to_longest(int block_num)
{
	return blockChain->toLongest(block_num);
}

/**
* DESCRIPTION: This function blocks all other Block attachments, until block_num is added
* to the chain. The block_num is the assigned value that was previously returned by add_block.
* RETURN VALUE: If block_num doesn't exist, return -2;
* In case of other errors, return -1; In case of success or if it is already attached return 0.
*/
int attach_now(int block_num)
{
	return blockChain->attachNow(block_num);

}
	
/**
* DESCRIPTION: Without blocking, check whether block_num was added to the chain.
* The block_num is the assigned value that was previously returned by add_block.
* RETURN VALUE: 1 if true and 0 if false. If the block_num doesn't exist, return -2; 
* In case of other errors, return -1.
*/
int was_added(int block_num)
{
	return blockChain->wasAdded(block_num);
}

/**
* DESCRIPTION: Return how many Blocks were attached to the chain since init_blockchain.
* If the chain was closed (by using close_chain) and then initialized (init_blockchain) 
* again this function should return the new chain size.
* RETURN VALUE: On success, the number of Blocks, otherwise -1.
*/
int chain_size()
{
	return blockChain->getSize();
}

/**
* DESCRIPTION: Search throughout the tree for sub-chains that are not the longest chain,
* detach them from the tree, free the blocks, and reuse the block_nums.
* RETURN VALUE: On success 0, otherwise -1.
*/
int prune_chain()
{	
	return blockChain->pruneChain();
}

/**
* DESCRIPTION: Close the recent blockchain and reset the system, so that it is possible to
* call init_blockchain again. Non-blocking. All pending Blocks should be hashed and printed 
* to terminal (stdout). Calls to library methods which try to alter the state of the Blockchain
* are prohibited while closing the Blockchain. e.g.: Calling chain_size() is ok, a call to 
* prune_chain() should fail. In case of a system error, the function should cause the
* process to exit.
*/
void close_chain()
{
	blockChain->closeChain();
}

/**
* DESCRIPTION: The function blocks and waits for close_chain to finish.
* RETURN VALUE: If closing was successful, it returns 0.
* If close_chain was not called it should return -2. In case of other error, 
* it should return -1.
*/
int return_on_close()
{
	return blockChain->returnOnClose();
}


