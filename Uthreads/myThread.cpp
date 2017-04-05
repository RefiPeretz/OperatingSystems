#include "myThread.hpp"

#ifdef __x86_64__
/* code for 64 bit Intel arch */

typedef unsigned long address_t;
#define JB_SP 6
#define JB_PC 7

/**
* A translation is required when using an address of a variable.
* Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%fs:0x30,%0\n"
		"rol    $0x11,%0\n"
                 : "=g" (ret)
                 : "0" (addr));
    return ret;
}

#else
/* code for 32 bit Intel arch */

typedef unsigned int address_t;
#define JB_SP 4
#define JB_PC 5 

/** A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%gs:0x18,%0\n"
		"rol    $0x9,%0\n"
                 : "=g" (ret)
                 : "0" (addr));
    return ret;
}

#endif

/** defulte constructor */
Thread::~Thread()
{

}
/** thread constructor */
Thread::Thread(void (*f)(void), Priority threadPr, unsigned int newTid)
{

			
	
	tid = newTid;
	pr = threadPr;
	quantums = 0;

	address_t stackPointer = (address_t)stack + STACK_SIZE - sizeof(address_t);
	programCounter = (address_t)f;
	sigsetjmp(threadEnv, 1);
	(threadEnv->__jmpbuf)[JB_SP] = translate_address(stackPointer);
	(threadEnv->__jmpbuf)[JB_PC] = translate_address(programCounter);
	sigemptyset(&threadEnv->__saved_mask);
	
}


/**
* A function that return the environment of the thread
*/
sigjmp_buf *Thread::getEnv()
{
	return &threadEnv;
}

/**
* A function that return the priority of the thread
*/
Priority Thread::getPriority()
{
	return pr;
}

/**
* A function that return the counter of the program
*/
address_t Thread::getProgramCounter()
{
	return programCounter;
}

struct timeval Thread::getReadyFrom()
{
	return readyFrom;
}

/**
* A function that return the state of the thread
*/
State Thread::getState()
{
	return state;
}

/**
* A function that return the id of the thread
*/	
unsigned int Thread::getTid()
{
	return tid;
}

/**
* input : stateThread - the new state we want 
* A function that setes the state of the thread
*/
void Thread::setState(State stateThread)
{
	state = stateThread;
}

/**
* A function that return the quantums of the thread
*/
unsigned int Thread::getQuantums()
{
	return quantums;
}

/**
* A function that the increase the number of the quantums the thread
*/
void Thread::incrementQuantumsCounter()
{
	quantums++;
}

