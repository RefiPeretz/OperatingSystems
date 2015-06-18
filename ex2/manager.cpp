#include "manager.hpp"
#include "myThread.hpp"


#define DEVIDE_FACTOR 100000
#define CONTINUE 1
#define RESET 0

Manager *Manager::programManager = NULL;

/** 
* constructor of the object that organize all the program
*/
Manager::Manager()
{
	countQuantums = RESET;
	countThreadsNum = RESET;
	for(int i = RESET; i < MAX_THREAD_NUM; i++)
	{
		threadArray[i] = NULL;
	}
	programManager = this;
}

/**
* input: thread - the thread we want to add.
* A function that add a new thread to the suitable priority queue
* return: 0 if succeeded to add the thread to the siutable queue, and
* 1 otherwise.
*/
int Manager::addThread(Thread *thread)
{

	threadArray[thread->getTid()] = thread;
	increaseTotalNumberOfThreads();
	switch(thread->getPriority())
	{
		case RED:
			redQueue.push_back(thread);
			break;
		case ORANGE:
			orangeQueue.push_back(thread);
			break;
		case GREEN:
			greenQueue.push_back(thread);
			break;

	}
	return OK;
}

/**
* description: a getter function
* return: number of total quantums in the program.
*/
int Manager::getTotalNumberOfQuantums() 
{ 
	return countQuantums; 
}

/**
* description: a getter function
* return: number of total threads in the program
*/
int Manager::getTotalNumberOfThreads() 
{
    return countThreadsNum;
}

/**
* description: A void function that increase the total 
* number of thereads in the program
* return: nothing.
*/
void Manager::increaseTotalNumberOfThreads()
{ 

	countThreadsNum++; 
}

/**
* description: A void function that decrease the total 
* number of thereads in the program
*/
void Manager::decreaseTotalNumberOfThreads()
{
	countThreadsNum--; 
}

/**
* description: A function that returns the minimal NULL index in the global
* array thread
* throw: ERROR if didnt find null indext in the array.
*/
unsigned int Manager::getMinTid()
{
	int i = 1; 
	while(i < MAX_THREAD_NUM)
	{ 
		if(threadArray[i] == NULL)
		{ 
			return i;
		} 
		i++; 
	} 
	//if cant find new avivable ID
	return ERROR; 
}

/**
* input: tid - id of a thread.
* description: a getter function
* return: the suitable thread from the global array of threads
* throw: ERROR if didnt found the given id.
*/
Thread *Manager::getThread(int tid)
{
	if(threadArray[tid] != NULL)
	{
		return threadArray[tid];
	}
	else
	{
		throw ERROR;
	}
}

/**
* input: thread - the thread we want to add to the ready queues.
* description: A funcrtion that add a tread to the suitable priority queue
* return: 0 if the function succeeded makeing the thread ready
*/
int Manager::readyThread(Thread *thread)
{
	if(thread->getState() != READY)
	{
		switch(thread->getPriority())
		{
			case RED:
				redQueue.push_back(thread);
				break;
			case ORANGE:

				orangeQueue.push_back(thread);
				break;
			case GREEN:
				greenQueue.push_back(thread);
				break;
		}
	}
	thread->setState(READY);
	return OK;	
}

/**
* input: tid - thread id
*        &q - queue address
* description: A void function that remove a thread from the given queue
*/
void Manager::removeThreadFromReady(std::deque<Thread*> &q, int tid)
{
	q.erase(std::remove(q.begin(), q.end(), threadArray[tid]), q.end());
}

/**
* input: quantum - number of quantums given in the bigining.
* description: A void function that set the quatum timer
*/
void Manager::setQuantumTimer(int quantum)
{
	suseconds_t usec = quantum % DEVIDE_FACTOR;
	time_t sec = (quantum - usec) / DEVIDE_FACTOR;
	quantumThread.it_value.tv_sec = sec;  	
	quantumThread.it_value.tv_usec = usec;    
	quantumThread.it_interval.tv_sec = sec;  	
	quantumThread.it_interval.tv_usec = usec;

}

/**
* description: A void function that increase total quatum counter 
*/
void Manager::increaseTotalQuantumCounter()
{
	countQuantums++;
}

/**
* description: A void function that switchs between the threads in the program
*/
void Manager::switchThreads()
{
  	sigset_t oldMask ,blockMask;
 	checkSystemError(sigfillset(&blockMask));
  	checkSystemError(sigprocmask(SIG_SETMASK, &blockMask, &oldMask));
	int ret_val = sigsetjmp(*(running->getEnv()),1);
	if (ret_val == 1) 
	{
		checkSystemError(sigprocmask(SIG_SETMASK, &oldMask, NULL));
	    return;
	}
	int readyId = getNextReadyId();
	//There is no next ready, continue with the current running
	if(readyId == ERROR)
	{
		readyId = running->getTid();
	}
	else if(running->getState() == TERMINATED)
	{

		delete(running);
	}

    else if(running != NULL && running->getState() != BLOCKED)
    {
    	running->setState(BLOCKED);
    	readyThread(running);
    }

	running = threadArray[readyId];
	increaseTotalQuantumCounter();
	running->incrementQuantumsCounter();
  	checkSystemError(sigprocmask(SIG_SETMASK, &oldMask, NULL));
	siglongjmp(*((threadArray[readyId])->getEnv()),1);
}


/**
* input: signal 
* description: A help void function that helps switching the threads in the program
*/
void Manager::staticSwitchThreads(int sig)
{	//avoiding complier warinning unused.
	(void) sig;
	(*programManager).switchThreads();
}

/**
* description: A getter function 
* return: number of quantum of the timer
*/
itimerval *Manager::getQuantumTimer()
{
	return &quantumThread;
}

/**
* description: A function that return the id thread that need to be running by priority
* return: ERROR if did not succeeded to get the next ready thread.
*/
int Manager::getNextReadyId()
{
	Thread* readyThread;
	if(!redQueue.empty())
	{
		readyThread = redQueue.front();
		redQueue.pop_front();
		return readyThread->getTid();
	}
	else if(!orangeQueue.empty())
	{
		readyThread = orangeQueue.front();
		orangeQueue.pop_front();
		return readyThread->getTid();
	}
	else if(!greenQueue.empty())
	{
		readyThread = greenQueue.front();
		greenQueue.pop_front();
		return readyThread->getTid();
	}
	return ERROR;

}

/**
* description: A void function that exit the program if the main thread is terminated.
*/
void Manager::shutdown()
{
	for(int i = 0; i < MAX_THREAD_NUM; i++)
	{
		if(threadArray[i] != NULL)
		{
			delete(threadArray[i]);
		}
	}

	delete(this);
}

/**
* check for signal erros.
*/
void Manager::checkSystemError(int retVal)
{
	if(retVal == ERROR)
	{		
		std::cerr << "system error: signal fail\n";
		exit(SYSTEM_ERROR);
	}
}
