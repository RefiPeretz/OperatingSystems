#ifndef _MY_THREAD_H
#define _MY_THREAD_H

#include <cstdlib>
#include <stdio.h>
#include <setjmp.h>
#include <signal.h>
#include <sys/time.h>
#include "uthreads.h"

#define OK 0
#define ERROR -1
#define SYSTEM_ERROR 1

typedef enum State { READY, RUNNING, BLOCKED, TERMINATED } State;

typedef unsigned long address_t;

class Thread
{
public:
	Thread(void (*f)(void), Priority pr, unsigned int newTid);
	~Thread();
	/* get thread id */
	unsigned int getTid();
	address_t getProgramCounter();
	/* get thread priority */
	Priority getPriority();
	/* get thread state */
	State getState();
	/* get thread quantums */
	unsigned int getQuantums();
	struct timeval getReadyFrom();
	/* get thread environment */
	sigjmp_buf *getEnv();
	void setReadyFrom();
	/* set thread state */
	void setState(State state);
	/* set quantums counter */
	void  incrementQuantumsCounter();
private:
	State state;
	Priority pr;
 	sigjmp_buf threadEnv;
	int stack[STACK_SIZE];
	address_t programCounter;
	struct timeval readyFrom;
	unsigned int quantums;
	unsigned int tid;

};

#endif