#ifndef _MANAGER_H
#define _MANAGER_H

#include <setjmp.h>
#include <signal.h>
#include <unistd.h>
#include <queue>
#include <sys/time.h>
#include <algorithm>
#include <cstdlib>
#include <map>
#include "myThread.hpp"
#include <iostream>
#include <deque>




#define SECOND 1000000



class Manager
{
	public:
		Manager();

		static Manager *programManager;
	    //struct itimerval quantumTimer;
	    /* get the next thread that need to run */
	    int getNextReadyId();
	    /* add new thread */
		int addThread(Thread *thread);
		/* move thread to the ready queue */
		int readyThread(Thread *thread);
		/* get thread from the given id*/
		Thread *getThread(int tid);
		Thread *running;
		/* get total number of quantums*/
		int getTotalNumberOfQuantums();
		/* get quantums timer*/
		itimerval *getQuantumTimer();
		/* get total number of thread */
		int getTotalNumberOfThreads();
		/* get the minimum thread id that is null */
		unsigned int getMinTid();
		/* remove thread */
		void removeThreadFromReady(std::deque<Thread*> &q, int tid);
		/* switch threads */
		static void staticSwitchThreads(int signal);
		/* set quantum timer*/
		void setQuantumTimer(int quantum);
		/* decrease total number of threads */
		void decreaseTotalNumberOfThreads();
		/* increase total number of quantum */
		void increaseTotalQuantumCounter();
		/* increase total number of thread */
		void increaseTotalNumberOfThreads();
		/* if deleteing thread 0 */
		void shutdown();
		/* switch threads */
		void switchThreads();
		void checkSystemError(int retVal);
		Thread *threadArray[MAX_THREAD_NUM];
		/* the priority queues */
		std::deque<Thread*> redQueue;
		std::deque<Thread*> orangeQueue;
		std::deque<Thread*> greenQueue;


	private:
		struct itimerval quantumThread;
		/* global counters */
		int countQuantums;
		int countThreadsNum;


};
#endif