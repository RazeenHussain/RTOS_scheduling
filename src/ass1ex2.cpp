/*******************************************************************************************\	
|				 REAL-TIME OPERATING SYSTEMS				    |
|					ASSIGNMENT # 1					    |
|				EXERCISE # 2 : EDF SCHEDULER				    |
|			        SUBMITTED BY : RAZEEN HUSSAIN				    | 
|				        EMARO+ STUDENT					    | 
|				     MATRICOLA : 4273095				    |
\*******************************************************************************************/

/* 
compile with
	g++ ass1ex2.cpp -lpthread -o ass1ex2
execute with
	./ass1ex2
*/

/*
Basic concepts:
- EDF is a dynamic algorithm which assigns the CPU computation to the task with nearest deadline
- EDF is an optimal algorithm
- For EDF Ulub = 1 and U = sum(Ci/Ti)
- Scheduling Theorem U <= Ulub
*/

/*
Logic used for the exercise:
- Three threads are created for the periodic tasks
- The worst case execution time for each task is calculated
- Schedulability condition for EDF algorithm is checked
- If task set is not schedulable, program exits
- Else the program continues
- A thread is created for the EDF scheduler
- This thread has the highest priority
- Whenever a task arrives, the edf_scheduler thread executes and calculates and assigns the priorities for each task based on the deadlines
- Then it goes to nanosleep until another task arrives
- Meanwhile the other task threads run as per the priorities assigned by the EDF scheduler
- Completion flags are used to moniter when the threads have been terminated
- When all threads have been executed, the edf_scheduler thread also terminates
*/

// Required header files
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>

// There are three periodic tasks with periods 100 ms, 200 ms, 400 ms 
// The hyperperiod is 400ms
// The deadlines of each periodic task is same as their periods

// Declaration of functions that contains the application code of each periodic task and their thread functions
void task1_code();
void task2_code();
void task3_code();
void *task1(void *);
void *task2(void *);
void *task3(void *); 

// Declaration of EDF scheduler thread function
void *edf_scheduler(void *);

// Function to compare deadlines of the tasks
bool less(int,int);

// Declaration of arrays, constants and variables to be used in the scheduling algorithm
#define NTASKS 3					// 3 periodic tasks need to be scheduled
long int periods[NTASKS];				// task periods
struct timeval next_arrival_time[NTASKS];		// arrival time in the following period
long int WCET[NTASKS];					// computational time of each task
pthread_attr_t attributes[NTASKS], attributesEDF;	// scheduling attributes
pthread_t thread_id[NTASKS],threadEDF;			// thread identifiers
struct sched_param parameters[NTASKS], parametersEDF;	// scheduling parameters
int missed_deadlines[NTASKS];				// number of missed deadline
bool completion_flag[NTASKS];				// completion flags
int current_task;					// flag to identify which task is currently running

// The main function
int main()
{
	system("clear");
	printf("\nEarliest Deadline First (EDF) Exercise By RAZEEN HUSSAIN\n\n\n");
	
	// setting task periods in nanoseconds
	periods[0]= 100000000;	// period for periodic task 1
	periods[1]= 200000000;	// period for periodic task 2
	periods[2]= 400000000;	// period for periodic task 3

	// initializing completion and current task flags
	for (int i=0; i<NTASKS; i++)
	{
		completion_flag[i]=false;
	}
	current_task=0;

	// declaring two variables priomin and priomax that are used to store the maximum and minimum priority
	struct sched_param priomax;
	priomax.sched_priority=sched_get_priority_max(SCHED_FIFO);
	struct sched_param priomin;
	priomin.sched_priority=sched_get_priority_min(SCHED_FIFO);

	// setting the maximum priority to the current thread
	if (getuid() == 0)
	{
		pthread_setschedparam(pthread_self(),SCHED_FIFO,&priomax);
	}

	// executing all tasks in standalone modality in order to measure execution times
	int i;
	for (i =0; i < NTASKS; i++)
	{
		struct timeval timeval1, timeval2;
		struct timezone timezone1, timezone2;
		gettimeofday(&timeval1, &timezone1);
		if (i==0)	// worst case scenario for period task 1 is when it task 1 executes
		{
			task1_code();
		}
		if (i==1)	// worst case scenario for period task 2 is when it task 2 executes
		{
			task2_code();
		}
		if (i==2)	// worst case scenario for period task 3 is when it task 3 executes
		{
			task3_code();
		}
		gettimeofday(&timeval2, &timezone2);
		WCET[i]= 1000*((timeval2.tv_sec - timeval1.tv_sec)*1000000 + (timeval2.tv_usec - timeval1.tv_usec));
		printf(")  Worst Case Execution Time For Periodic Task %d = %ld \n", (i+1), WCET[i]);
	}

	// computing U
 	double U = 0;
  	for (int i = 0; i < NTASKS; i++)
    	{
		U+= ((double)WCET[i])/((double)periods[i]);
	}
	
	// computing Ulub
	double Ulub = 1;

	// verifying schedulability of the task set
	if (U > Ulub)
	{
		printf("\n U = %lf   Ulub = %lf   \n\tNon schedulable Task Set\n\n", U, Ulub);
		return(-1);
	}
	printf("\n U = %lf   Ulub = %lf   \n\tScheduable Task Set\n", U, Ulub);
	printf("\nStarting EDF Scheduling Algorithm\n");
	fflush(stdout);
	sleep(5);
	printf("\nExecuting Periodic Tasks : \n");

	// setting the minimum priority to the current thread
	if (getuid() == 0)
	{
		pthread_setschedparam(pthread_self(),SCHED_FIFO,&priomin);
	}

	// setting the attributes of each task, including scheduling policy and priority
	for (i =0; i < NTASKS; i++)
	{
		pthread_attr_init(&(attributes[i]));
		pthread_attr_setinheritsched(&(attributes[i]), PTHREAD_EXPLICIT_SCHED);
		pthread_attr_setschedpolicy(&(attributes[i]), SCHED_FIFO);
		parameters[i].sched_priority = priomin.sched_priority+NTASKS - i;
		pthread_attr_setschedparam(&(attributes[i]), &(parameters[i]));
	}

	// reading the current time and setting the next arrival time for periodic each task (i.e., the beginning of the next period)
	struct timeval ora;
	struct timezone zona;
	gettimeofday(&ora, &zona);
	for (i = 0; i < NTASKS; i++)
	{
		long int periods_micro = periods[i]/1000;
		next_arrival_time[i].tv_sec = ora.tv_sec + periods_micro/1000000;
		next_arrival_time[i].tv_usec = ora.tv_usec + periods_micro%1000000;
		missed_deadlines[i] = 0;
	}

	// setting attributes for the EDF scheduler thread
	pthread_attr_init(&(attributesEDF));
	pthread_attr_setinheritsched(&(attributesEDF), PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&(attributesEDF), SCHED_FIFO);
	parametersEDF.sched_priority = 5;
	pthread_attr_setschedparam(&(attributesEDF), &(parametersEDF));

	// creating all threads
	int iret[NTASKS], edf;
	iret[0] = pthread_create( &(thread_id[0]), &(attributes[0]), task1, NULL);
	iret[1] = pthread_create( &(thread_id[1]), &(attributes[1]), task2, NULL);
	iret[2] = pthread_create( &(thread_id[2]), &(attributes[2]), task3, NULL);
	edf = pthread_create( &(threadEDF), &(attributesEDF), edf_scheduler, NULL);

	// joining all threads
	pthread_join( thread_id[0], NULL);
	pthread_join( thread_id[1], NULL);
	pthread_join( thread_id[2], NULL);
	pthread_join( threadEDF, NULL);

	exit(0);
}

// application specific task_1 code
void task1_code()
{
	int i,j;
	for (i = 0; i < 10; i++)
	{
		for (j = 0; j < 10000; j++)
		{
			current_task=0;
			double var = rand()*rand();
    		}
  	}
	printf(" 1");
  	fflush(stdout);
}

// application specific task_2 code
void task2_code()
{
	double var;
	int i;
	for (i = 0; i < 10; i++)
	{
		int j;
		for (j = 0; j < 10000; j++)
		{
			current_task=1;
			var = (rand()*rand())%10;
		}
	}
	printf(" 2");
  	fflush(stdout);
}

// application specific task_3 code
void task3_code()
{
	double var;
	int i;
  	for (i = 0; i < 10; i++)
    	{
		int j;
      		for (j = 0; j < 10000; j++);		
      		{
			current_task=2;
			var = (rand()*rand())%10;
		}
	}
  	printf(" 3");
  	fflush(stdout);
}

//thread code for task_1 (used only for temporization)
void *task1( void *ptr)
{
  	int i=0;
  	struct timespec waittime;
  	waittime.tv_sec=0;
  	waittime.tv_nsec = periods[0];
	for (i=0; i < 100; i++)
	{
      		// executing application specific code
		task1_code();
		struct timeval ora;
      		struct timezone zona;
      		gettimeofday(&ora, &zona);
      		// after execution, computing the time to the beginning of the next period
      		long int timetowait= 1000*((next_arrival_time[0].tv_sec - ora.tv_sec)*1000000+(next_arrival_time[0].tv_usec-ora.tv_usec));
      		if (timetowait < 0)
		{
			missed_deadlines[0]++;
		}
      		waittime.tv_sec = timetowait/1000000000;
      		waittime.tv_nsec = timetowait%1000000000;
   	   	nanosleep(&waittime, NULL);		// suspending the task until the beginning of the next period
      		// the task is ready: setting the next arrival time for the task (i.e., the beginning of the next period)
      		long int periods_micro=periods[0]/1000;
      		next_arrival_time[0].tv_sec = next_arrival_time[0].tv_sec + periods_micro/1000000;
      		next_arrival_time[0].tv_usec = next_arrival_time[0].tv_usec + periods_micro%1000000;
	}
	completion_flag[0]=true;
}

//thread code for task_2 (used only for temporization)
void *task2( void *ptr)
{
  	int i=0;
  	struct timespec waittime;
  	waittime.tv_sec=0;
  	waittime.tv_nsec = periods[1];
	for (i=0; i < 100; i++)
	{
      		// executing application specific code
		task2_code();
		struct timeval ora;
      		struct timezone zona;
      		gettimeofday(&ora, &zona);
      		// after execution, computing the time to the beginning of the next period
      		long int timetowait= 1000*((next_arrival_time[1].tv_sec - ora.tv_sec)*1000000+(next_arrival_time[1].tv_usec-ora.tv_usec));
      		if (timetowait < 0)
		{
			missed_deadlines[1]++;
		}
      		waittime.tv_sec = timetowait/1000000000;
      		waittime.tv_nsec = timetowait%1000000000;
   	   	nanosleep(&waittime, NULL);		// suspending the task until the beginning of the next period
      		// the task is ready: setting the next arrival time for the task (i.e., the beginning of the next period)
      		long int periods_micro=periods[1]/1000;
      		next_arrival_time[1].tv_sec = next_arrival_time[1].tv_sec + periods_micro/1000000;
      		next_arrival_time[1].tv_usec = next_arrival_time[1].tv_usec + periods_micro%1000000;
	}
	completion_flag[1]=true;
}

//thread code for task_3 (used only for temporization)
void *task3( void *ptr)
{
  	int i=0;
  	struct timespec waittime;
  	waittime.tv_sec=0;
  	waittime.tv_nsec = periods[2];
	for (i=0; i < 100; i++)
	{
      		// executing application specific code
		task3_code();
		struct timeval ora;
      		struct timezone zona;
      		gettimeofday(&ora, &zona);
      		// after execution, computing the time to the beginning of the next period
      		long int timetowait= 1000*((next_arrival_time[2].tv_sec - ora.tv_sec)*1000000+(next_arrival_time[2].tv_usec-ora.tv_usec));
      		if (timetowait < 0)
		{
			missed_deadlines[2]++;
		}
      		waittime.tv_sec = timetowait/1000000000;
      		waittime.tv_nsec = timetowait%1000000000;
   	   	nanosleep(&waittime, NULL);		// suspending the task until the beginning of the next period
      		// the task is ready: setting the next arrival time for the task (i.e., the beginning of the next period)
      		long int periods_micro=periods[2]/1000;
      		next_arrival_time[2].tv_sec = next_arrival_time[2].tv_sec + periods_micro/1000000;
      		next_arrival_time[2].tv_usec = next_arrival_time[2].tv_usec + periods_micro%1000000;
	}
	completion_flag[2]=true;
}

// returns true if deadline of A is less than that of B
bool less(int A,int B)
{
	long int perAsec,perBsec;
	long int perAusec,perBusec;
	perAsec=periods[A]/1000000000;
	perAusec=(periods[A]%1000000000)/1000;
	perBsec=periods[B]/1000000000;
	perBusec=(periods[B]%1000000000)/1000;
	if ((next_arrival_time[A].tv_sec+perAsec) < (next_arrival_time[B].tv_sec+perBsec))
	{
		return true;
	}
	else if ((next_arrival_time[A].tv_sec+perAsec) > (next_arrival_time[B].tv_sec+perBsec))
	{
		return false;
	}
	else
	{
		if ((next_arrival_time[A].tv_usec+perAsec) < (next_arrival_time[B].tv_usec+perBusec))
		{
			return true;
		}
		else
		{
			return false;
		}
	}
}

// thread code for the EDF scheduler
void *edf_scheduler(void *)
{
 	struct timespec waittime;
  	waittime.tv_sec=0;
  	waittime.tv_nsec = 0;
	int nearest=0;
	int middle=1;
	int furthest=2;
	while (!((completion_flag[0]) & (completion_flag[1]) & (completion_flag[2])))
	{
		// identifying task with nearest deadline and assigning priorities accordingly
		if (less(0,1))
		{
			if (less(0,2))
			{
				nearest=0; 
				if (less(1,2))
				{
					middle=1;
					furthest=2;
				}
				else
				{
					middle=2;
					furthest=1;
				}
			}
			else
			{
				middle=0;
				if (less(1,2))
				{
					nearest=1;
					furthest=2;
				}
				else
				{
					nearest=2;
					furthest=1;
				}
			}
		}
		else
		{
			if (less(1,2))
			{
				nearest=1;
				if (less(0,2))
				{
					middle=0;
					furthest=2;
				}
				else
				{
					middle=2;
					furthest=0;
				}
			}
			else
			{
				nearest=2;
				if (less(0,2))
				{
					middle=0;
					furthest=1;
				}
				else
				{
					middle=1;
					furthest=0;
				}

			}
		}

		// re-assigning priorities
		parameters[middle].sched_priority = 4;
		pthread_attr_setschedparam(&(attributes[nearest]), &(parameters[nearest]));
		parameters[middle].sched_priority = 3;
		pthread_attr_setschedparam(&(attributes[middle]), &(parameters[middle]));
		parameters[furthest].sched_priority = 2;
		pthread_attr_setschedparam(&(attributes[furthest]), &(parameters[furthest]));
		
		// calculating time till next task arrival 
		struct timeval ora;
      		struct timezone zona;
      		gettimeofday(&ora, &zona);
      		long int timetowait= 1000*((next_arrival_time[current_task].tv_sec - ora.tv_sec)*1000000+(next_arrival_time[current_task].tv_usec-ora.tv_usec));
      		waittime.tv_sec = timetowait/1000000000;
      		waittime.tv_nsec = timetowait%1000000000;
      		waittime.tv_nsec += periods[current_task];
   	   	nanosleep(&waittime, NULL);		// suspending the task until the arrival of the next task
	}
	printf("\n\nTask Execution Completed\n\nExiting EDF Scheduler\n\n\n");
}

