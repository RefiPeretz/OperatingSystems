#include "manager.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <queue>
#include <cstdlib>
#include <map>
#include <sys/time.h>
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>
#include <assert.h>
#include <iostream>
#include <deque>
#include <list>

using namespace std;

Manager* gManager;

/** 
* input: quantum_usecs - number of quantums for every clock stop
* description: Initialize the thread library 
*/
int uthread_init(int quantum_usecs)
{

  if(quantum_usecs <= 0)
  {
    std::cerr << "thread library error: less or equal to 0 quantum usecs\n";
    return ERROR;
  }
	gManager = new Manager();
	gManager->setQuantumTimer(quantum_usecs);
	//Creating therad 0
	Thread *mainThread = new Thread(NULL, ORANGE ,0);

	gManager->threadArray[0] = mainThread;

	gManager->running = mainThread;

	gManager->increaseTotalNumberOfThreads();

  gManager->increaseTotalQuantumCounter();

  gManager->running->incrementQuantumsCounter();


//Creating signal
  struct sigaction actionCatcher;
  actionCatcher.sa_handler = Manager::staticSwitchThreads;
  gManager->checkSystemError(sigaction(SIGVTALRM, &actionCatcher, NULL));

	gManager->checkSystemError(setitimer(ITIMER_VIRTUAL, gManager->getQuantumTimer(), NULL));

  return OK;
}

/** 
* input: pr - the priority of the the new thread.
*        *f - the thread function
* description: Create a new thread whose entry point is f 
* return: ERROR if passed the maxmimum number of threads.
*/
int uthread_spawn(void (*f)(void), Priority pr)
{

  sigset_t oldMask ,blockMask;
  gManager->checkSystemError(sigfillset(&blockMask));
  gManager->checkSystemError(sigprocmask(SIG_SETMASK, &blockMask, &oldMask));

	if(gManager->getTotalNumberOfThreads() >= MAX_THREAD_NUM)
	{
    std::cerr << "thread library error: maximum threads\n";
    gManager->checkSystemError(sigprocmask(SIG_SETMASK, &oldMask, NULL));
		return ERROR;
	}

  int id = gManager->getMinTid();
	Thread *newThread = new Thread(f, pr, id);
	newThread->setState(READY);
	gManager->addThread(newThread);
  gManager->checkSystemError(sigprocmask(SIG_SETMASK, &oldMask, NULL));
	return id;

}

/** 
* input: tid - the thread id we want to terminate.
* description: Terminate a thread 
* return: ERROR if didnt find the thread we want to terminate
*/
int uthread_terminate(int tid)
{

  sigset_t oldMask ,blockMask;
  gManager->checkSystemError(sigfillset(&blockMask));
  gManager->checkSystemError(sigprocmask(SIG_SETMASK, &blockMask, &oldMask));
  if(gManager->threadArray[tid] == NULL || tid < 0)
  {
    std::cerr << "thread library error: no such thread\n";
    gManager->checkSystemError(sigprocmask(SIG_SETMASK, &oldMask, NULL));
    return ERROR;
  }

  if(tid == 0)
    {
      gManager->checkSystemError(sigprocmask(SIG_SETMASK, &oldMask, NULL));
      gManager->shutdown();
      exit(OK);
    }

  Thread* deleteThread = gManager->threadArray[tid];

  if (deleteThread->getState() == READY)
  {
    switch(deleteThread->getPriority())
    {
      case RED:
        gManager->removeThreadFromReady(gManager->redQueue,deleteThread->getTid());
        break;
      case ORANGE:
        gManager->removeThreadFromReady(gManager->orangeQueue,deleteThread->getTid());
        break;
      case GREEN:
        gManager->removeThreadFromReady(gManager->greenQueue,deleteThread->getTid());
        break;
    }
  }
  deleteThread->setState(TERMINATED);
  gManager->threadArray[tid] = NULL;
  gManager->decreaseTotalNumberOfThreads();

  unsigned unSignedTid = tid;
  if(gManager->running->getTid() == unSignedTid)
  {
    //Will be deleted in Switch threads.
    gManager->checkSystemError(sigprocmask(SIG_SETMASK, &oldMask, NULL));
    gManager->switchThreads();
    return OK;
  }
  delete(deleteThread);
  gManager->checkSystemError(sigprocmask(SIG_SETMASK, &oldMask, NULL));
  return OK;
}

/** 
* input: tid - number of thread that we want to block
* description: Suspend a thread
* return: ERROR if we want to block the main thread (0), or we want
* to block a thread that does not exist 
*/
int uthread_suspend(int tid)
{
  sigset_t oldMask ,blockMask;
  gManager->checkSystemError(sigfillset(&blockMask));
  gManager->checkSystemError(sigprocmask(SIG_SETMASK, &blockMask, &oldMask));

  if(gManager->threadArray[tid] == NULL || tid <= 0)
  {
    if(tid == 0)
    {
      std::cerr << "thread library error: cannot suspend main thread\n";
      gManager->checkSystemError(sigprocmask(SIG_SETMASK, &oldMask, NULL));
      return ERROR;
    }
    std::cerr << "thread library error: no such thread\n";
    gManager->checkSystemError(sigprocmask(SIG_SETMASK, &oldMask, NULL));
    return ERROR;
  }
  if(gManager->threadArray[tid]->getState() == BLOCKED)
  {
    gManager->checkSystemError(sigprocmask(SIG_SETMASK, &oldMask, NULL));
    return OK;
  }

  if(gManager->threadArray[tid]->getState() == READY)
  {
    switch(gManager->threadArray[tid]->getPriority())
    {
      case RED:
        gManager->removeThreadFromReady(\
          gManager->redQueue,gManager->threadArray[tid]->getTid());
        break;
      case ORANGE:
        gManager->removeThreadFromReady(\
          gManager->orangeQueue,gManager->threadArray[tid]->getTid());
        break;
      case GREEN:
        gManager->removeThreadFromReady(\
          gManager->greenQueue,gManager->threadArray[tid]->getTid());
        break;
    }
  }
  gManager->threadArray[tid]->setState(BLOCKED);

  if(gManager->threadArray[tid] == gManager->running)
  {
    gManager->checkSystemError(sigprocmask(SIG_SETMASK, &oldMask, NULL));
    gManager->switchThreads();
    return OK;  
  }
  gManager->checkSystemError(sigprocmask(SIG_SETMASK, &oldMask, NULL));
  return OK;

}

/** 
* input: tid - the thread id that we want to resume from block.
* description: Resume a thread
* return: ERROR if we want to resume an unexisted thread. 
*/
int uthread_resume(int tid)
{
  sigset_t oldMask ,blockMask;
  gManager->checkSystemError(sigfillset(&blockMask));
  gManager->checkSystemError(sigprocmask(SIG_SETMASK, &blockMask, &oldMask));
  if(gManager->threadArray[tid] == NULL || tid < 0)
  {
    std::cerr << "thread library error: no such thread\n";
    gManager->checkSystemError(sigprocmask(SIG_SETMASK, &oldMask, NULL));
    return ERROR;
  }

  if(gManager->threadArray[tid]->getState() != READY)
  {

    gManager->readyThread(gManager->threadArray[tid]);
  }

  gManager->checkSystemError(sigprocmask(SIG_SETMASK, &oldMask, NULL));

  return OK;

}


/** 
* description: a getter function
* return: the id of the calling thread 
*/
int uthread_get_tid()
{
	return gManager->running->getTid();
}

/** 
* description: a getter function
* return: the total number of library quantums 
*/
int uthread_get_total_quantums()
{
	return gManager->getTotalNumberOfQuantums();
}

/** 
* input: tid - the thread id
* description: a getter function
* return: the number of thread quantums 
*/
int uthread_get_quantums(int tid)
{

  if(gManager->threadArray[tid] == NULL || tid < 0)
  {
    std::cerr << "thread library error: no such thread\n";
    return ERROR;
  }

	return gManager->threadArray[tid]->getQuantums();
}





