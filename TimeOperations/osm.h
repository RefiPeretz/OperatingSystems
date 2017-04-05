#ifndef _OSM_H
#define _OSM_H

#include <limits.h>

/* calling a system call that does nothing.
This system call always succeeds */
#define OSM_NULLSYSCALL asm volatile( "int $0x80 " : : \
        "a" (0xffffffff) /* no such syscall */, "b" (0), "c" (0), "d" (0) /*:\
        "eax", "ebx", "ecx", "edx"*/)


/* Initialization function that the user must call
 * before running any other library function.
 * Returns 0 uppon success and -1 on failure.
 */
int osm_init();

/* Time measurement function for an empty function call.
   returns time in nano-seconds upon success, 
   and -1 upon failure.
   Zero iterations number is invalid.
   */
double osm_function_time(unsigned int osm_iterations);

/* Time measurement function for an empty trap into the operating system.
   returns time in nano-seconds upon success, 
   and -1 upon failure.
   Zero iterations number is invalid.

   */
double osm_syscall_time(unsigned int osm_iterations);


/* Time measurement function for a simple arithmetic operation.
   returns time in nano-seconds upon success,
   and -1 upon failure.
   Zero iterations number is invalid.
   */
double osm_operation_time(unsigned int osm_iterations);

typedef struct {
	char machineName[HOST_NAME_MAX];
	int numberOfIterations;
	double instructionTimeNanoSecond;
	double functionTimeNanoSecond; 
	double trapTimeNanoSecond;
	double functionInstructionRatio;
	double trapInstructionRatio;	
} timeMeasurmentStructure;

timeMeasurmentStructure measureTimes (unsigned int osm_iterations);

#endif
