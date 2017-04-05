#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include "osm.h"



#define DEAFULT_VALUE 50000
#define SECONDES_TO_NANO 1000000000
#define MICRO_SECONDES_TO_NANO 1000
#define UNROOLING_FACTOR 8
#define DEAFULT_INSTRUCTION 2
#define FAILED -1


/* Initialization function that the user must call
 * before running any other library function.
 * Returns 0 uppon success and -1 on failure.
 */
int osm_init()
{
	return 0;
}


/* Time measurement function for a simple arithmetic operation.
   returns time in nano-seconds upon success,
   and -1 upon failure.
   Zero iterations number is invalid.
   */
double osm_operation_time(unsigned int osm_iterations)
{
	try
	{
		timeval begin, end;
		double elapsedTime;
	    double totalTime = 0;



		unsigned int i = 0;
	    gettimeofday(&begin, NULL);
		for(i = 0; i < osm_iterations; i += UNROOLING_FACTOR)
		{

			__asm__ ( "movl $1, %eax;""movl $2, %ebx;""addl %ebx, %eax;");
			__asm__ ( "movl $1, %eax;""movl $2, %ebx;""addl %ebx, %eax;");
			__asm__ ( "movl $1, %eax;""movl $2, %ebx;""addl %ebx, %eax;");
			__asm__ ( "movl $1, %eax;""movl $2, %ebx;""addl %ebx, %eax;");
			__asm__ ( "movl $1, %eax;""movl $2, %ebx;""addl %ebx, %eax;");
			__asm__ ( "movl $1, %eax;""movl $2, %ebx;""addl %ebx, %eax;");
			__asm__ ( "movl $1, %eax;""movl $2, %ebx;""addl %ebx, %eax;");
			__asm__ ( "movl $1, %eax;""movl $2, %ebx;""addl %ebx, %eax;");




		}
		gettimeofday(&end, NULL);

		elapsedTime = (end.tv_sec - begin.tv_sec) * SECONDES_TO_NANO;    
		elapsedTime += (end.tv_usec - begin.tv_usec) * MICRO_SECONDES_TO_NANO; 
		totalTime += elapsedTime;

		return totalTime / osm_iterations;

	}
	catch(...)
	{
		return FAILED;
	}



}

void emptyFunction()
{

}

/* Time measurement function for an empty function call.
   returns time in nano-seconds upon success, 
   and -1 upon failure.
   Zero iterations number is invalid.
   */
double osm_function_time(unsigned int osm_iterations)
{

	try
	{
		timeval begin, end;
		double elapsedTime;
	    double totalTime = 0;


		unsigned int i = 0;
	    gettimeofday(&begin, NULL);
		for(i = 0; i < osm_iterations; i += UNROOLING_FACTOR)
		{
			emptyFunction();
			emptyFunction();
			emptyFunction();
			emptyFunction();
			emptyFunction();
			emptyFunction();
			emptyFunction();
			emptyFunction();

	


		}
		gettimeofday(&end, NULL);
		elapsedTime = (end.tv_sec - begin.tv_sec) * SECONDES_TO_NANO;    
		elapsedTime += (end.tv_usec - begin.tv_usec) * MICRO_SECONDES_TO_NANO; 
		totalTime += elapsedTime;

		return totalTime / osm_iterations;

	}
	catch(...)
	{
		return FAILED;
	}



}


/* Time measurement function for an empty trap into the operating system.
   returns time in nano-seconds upon success, 
   and -1 upon failure.
   Zero iterations number is invalid.

   */
double osm_syscall_time(unsigned int osm_iterations)
{
	try
	{
		timeval begin, end;
		double elapsedTime;
	    double totalTime = 0;
	    //Round up.

		unsigned int i = 0;
	    gettimeofday(&begin, NULL);
		for(i = 0; i < osm_iterations; i += UNROOLING_FACTOR)
		{
			OSM_NULLSYSCALL;
			OSM_NULLSYSCALL;
			OSM_NULLSYSCALL;
			OSM_NULLSYSCALL;
			OSM_NULLSYSCALL;
			OSM_NULLSYSCALL;
			OSM_NULLSYSCALL;
			OSM_NULLSYSCALL;



		}
		gettimeofday(&end, NULL);
		elapsedTime = (end.tv_sec - begin.tv_sec) * SECONDES_TO_NANO;    
		elapsedTime += (end.tv_usec - begin.tv_usec) * MICRO_SECONDES_TO_NANO;
		totalTime += elapsedTime;

		return totalTime / osm_iterations;

	}
	catch(...)
	{
		return FAILED;
	}



}


/*	Measure the time of the functions and updatdting the struct values.

*/
timeMeasurmentStructure measureTimes (unsigned int osm_iterations = DEAFULT_VALUE)
{
	timeMeasurmentStructure timeStruct;

	int error = gethostname(timeStruct.machineName, HOST_NAME_MAX);
	if(error == FAILED)
	{
		strcpy(timeStruct.machineName, "");
	}


	//Unvalid iterations number.
	if(osm_iterations <= 0)
	{
		timeStruct.instructionTimeNanoSecond = FAILED;
		timeStruct.functionTimeNanoSecond = FAILED; 
		timeStruct.trapTimeNanoSecond = FAILED;
		timeStruct.functionInstructionRatio = FAILED;
		timeStruct.trapInstructionRatio = FAILED;	

	}

	//Round up according to our UNROLLING FACTOR.
    if(osm_iterations % UNROOLING_FACTOR != 0)
    {
    	osm_iterations += UNROOLING_FACTOR -(osm_iterations % UNROOLING_FACTOR);
    }


	//double emptyLoopTime = calculateLoopTime(osm_iterations);


    timeStruct.numberOfIterations = osm_iterations;

    timeStruct.instructionTimeNanoSecond = osm_operation_time(osm_iterations);

    timeStruct.functionTimeNanoSecond = osm_function_time(osm_iterations);

    timeStruct.trapTimeNanoSecond = osm_syscall_time(osm_iterations);

    timeStruct.functionInstructionRatio = timeStruct.functionTimeNanoSecond\
     / timeStruct.instructionTimeNanoSecond; 

    timeStruct.trapInstructionRatio = timeStruct.trapTimeNanoSecond\
     / timeStruct.instructionTimeNanoSecond;


	return timeStruct;


}

