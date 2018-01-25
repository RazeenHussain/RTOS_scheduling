/*******************************************************************************************\	
|				 REAL-TIME OPERATING SYSTEMS				    |
|					ASSIGNMENT # 1					    |
|				EXERCISE # 1 : POLLING SERVER				    |
|			        SUBMITTED BY : RAZEEN HUSSAIN				    | 
|				        EMARO+ STUDENT					    | 
|				     MATRICOLA = 4273095				    |
\*******************************************************************************************/

/* 
compile with
	g++ ass1ex1.cpp -lpthread -o ass1ex1
execute with
	./ass1ex1
*/

/*
Basic concepts:
- Polling server is executed at regular intervals to cater for the arrival of aperiodic tasks
- Polling server is characterized by its period Ts and its capacity Cs
- It is usually scheduled as a periodic task with highest priority and the rate monotonic algorithm
- If there are no pending requests, the residual capacity of the server is lost until the next period
- For polling server, U = sum(Ci/Ti)+Cs/Ts and Ulub = n*(pow(2.0,(1.0/n))-1)
- Schedulability test is U <= Ulub
*/

/*
Logic used in the exercise:
- Threads are created for the three periodic tasks and for the polling server
- Polling server is treated as a periodic task with lowest period and highest priority
- The task set is scheduled with rate monotonic
- The algorithm assumes the capacity of the server is sufficient to cater for the both aperiodic tasks
- Worst case execution times for each periodic tasks is calculated
- For polling server, the Cs is calculated as the sum of WCETs of the aperiodic tasks
- Schedulability condition is checked
- If it fails, the program terminates
- Else, the tasks start execution
- The arrival of aperiodic task 4 is controlled by periodic task 2
- The arrival of aperiodic task 5 is controlled by periodic task 3
- Completion flags are used to moniter the completion of periodic threads
- The polling server thread executes till all the periodic tasks threads have been completed
*/

// Required header files
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>

// There are three periodic tasks with periods 200 ms, 300 ms, 400 ms and a polling server with period 100ms
// The hyperperiod is 1200ms
// Polling server is a periodic task with highest priority (minimum period) which caters for the aperiodic tasks
// The deadlines of each periodic task is same as their periods

// Declaration of functions that contains the application code of each task and their thread functions
// Task 1,2,3 are periodic tasks
// Task 4,5 are aperiodic tasks
void task1_code();
void task2_code();
void task3_code();
void task4_code();
void task5_code();
void pollingServer_code();
void *task1(void *);
void *task2(void *);
void *task3(void *); 
void *pollingServer(void *);

//Declaration of arrays, constants and variables to be used in the scheduling algorithm
#define NTASKS 4				// 4 tasks need to be scheduled (the polling server and three periodic tasks)
long int periods[NTASKS];			// task periods
struct timeval next_arrival_time[NTASKS];	// arrival time in the following period
long int WCET[NTASKS];				// computational time of each task
pthread_attr_t attributes[NTASKS];		// scheduling attributes
pthread_t thread_id[NTASKS];			// thread identifiers
struct sched_param parameters[NTASKS];		// scheduling parameters
int missed_deadlines[NTASKS];			// number of missed deadline
int aperiodic4_flag, aperiodic5_flag;		// flags indicating whether the aperiodic tasks have arrived or not
bool completion_flag[(NTASKS-1)];		// completion flags

int main()
{
	system("clear");
	printf("\nPolling Server Exercise By RAZEEN HUSSAIN\n\n\n");

	// setting task periods in nanoseconds
	periods[0]= 100000000;	// period for the polling server
	periods[1]= 200000000;	// period for periodic task 1
	periods[2]= 300000000;	// period for periodic task 2
	periods[3]= 400000000;	// period for periodic task 3

	// initializing completion and current task flags
	for (int i=0; i<NTASKS; i++)
	{
		completion_flag[i]=false;
	}

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
		if (i==0)	// worst case scenario for polling server is when it has to cater both aperiodic tasks in the same period
		{
			task4_code();
			task5_code();
		}
		if (i==1)	// worst case scenario for period task 1 is when it task 1 executes
		{
			task1_code();
		}
		if (i==2)	// worst case scenario for period task 2 is when it task 2 executes
		{
			task2_code();
		}
		if (i==3)	// worst case scenario for period task 3 is when it task 3 executes
		{
			task3_code();
		}
		gettimeofday(&timeval2, &timezone2);
		WCET[i]= 1000*((timeval2.tv_sec - timeval1.tv_sec)*1000000 + (timeval2.tv_usec - timeval1.tv_usec));
		if (i==0)
		{			
			printf(")  Server Capacity (Sum of WCET For Aperiodic Tasks 4 & 5) = %ld \n", WCET[i]);
		}
		else
		{			
			printf(")  Worst Case Execution Time For Periodic Task %d = %ld \n", i, WCET[i]);
		}
	}

	// computing U
 	double U = 0;
  	for (int i = 0; i < NTASKS; i++)
    	{
		U+= ((double)WCET[i])/((double)periods[i]);
	}
	
	// computing Ulub
	double Ulub = NTASKS*(pow(2.0,(1.0/NTASKS)) -1);

	// verifying schedulability of the task set
	if (U > Ulub)
	{
		printf("\n U = %lf   Ulub = %lf   \n\tNon schedulable Task Set\n", U, Ulub);
		return(-1);
	}
	printf("\n U = %lf   Ulub = %lf   \n\tScheduable Task Set\n", U, Ulub);
	printf("\n Starting Scheduler\n");	
	fflush(stdout);
	sleep(5);

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

	aperiodic4_flag=0;
	aperiodic5_flag=0;
	printf("\nExecuting Tasks : \n");	

	// creating all threads
	int iret[NTASKS];
	iret[0] = pthread_create( &(thread_id[0]), &(attributes[0]), pollingServer, NULL);
	iret[1] = pthread_create( &(thread_id[1]), &(attributes[1]), task1, NULL);
	iret[2] = pthread_create( &(thread_id[2]), &(attributes[2]), task2, NULL);
	iret[3] = pthread_create( &(thread_id[3]), &(attributes[3]), task3, NULL);

	// joining all threads
	pthread_join( thread_id[0], NULL);
	pthread_join( thread_id[1], NULL);
	pthread_join( thread_id[2], NULL);
	pthread_join( thread_id[3], NULL);

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
			var = (rand()*rand())%10;
		}
	}
	// condition for arrival of aperiodic task 4
	if (var < 2)
	{
		aperiodic4_flag=1;
	}
	else
	{
		aperiodic4_flag=0;
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
			var = (rand()*rand())%10;
		}
	}
	// condition for arrival of aperiodic task 5
	if (var < 2)
	{
		aperiodic5_flag=1;
	}
	else
	{
		aperiodic5_flag=0;
	}
  	printf(" 3");
  	fflush(stdout);
}

// application specific task_4 code
void task4_code()
{
	int i;
  	for (i = 0; i < 10; i++)
    	{
		int j;
      		for (j = 0; j < 10000; j++);		
      		{
			double var = rand()*rand();
		}
	}
	printf("4");
  	fflush(stdout);
}

// application specific task_5 code
void task5_code()
{
	int i;
  	for (i = 0; i < 10; i++)
    	{
		int j;
      		for (j = 0; j < 10000; j++);		
      		{
			double var = rand()*rand();
		}
	}
	printf("5");
  	fflush(stdout);
}

// application specific polling_server code
void pollingServer_code()
{
	printf(" p");
	if (aperiodic4_flag == 1)
	{
		task4_code();
		aperiodic4_flag=0;
	}
	if (aperiodic5_flag == 1)
	{
		task5_code();
		aperiodic5_flag=0;
	}
  	fflush(stdout);
}

//thread code for task_1 (used only for temporization)
void *task1( void *ptr)
{
  	int i=0;
  	struct timespec waittime;
  	waittime.tv_sec=0;
  	waittime.tv_nsec = periods[1];
	for (i=0; i < 100; i++)
	{
      		// executing application specific code
		task1_code();
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
	completion_flag[0]=true;
}

//thread code for task_2 (used only for temporization)
void *task2( void *ptr)
{
  	int i=0;
  	struct timespec waittime;
  	waittime.tv_sec=0;
  	waittime.tv_nsec = periods[2];
	for (i=0; i < 100; i++)
	{
      		// executing application specific code
		task2_code();
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
	completion_flag[1]=true;
}

//thread code for task_3 (used only for temporization)
void *task3( void *ptr)
{
  	int i=0;
  	struct timespec waittime;
  	waittime.tv_sec=0;
  	waittime.tv_nsec = periods[3];
	for (i=0; i < 100; i++)
	{
      		// executing application specific code
		task3_code();
		struct timeval ora;
      		struct timezone zona;
      		gettimeofday(&ora, &zona);
      		// after execution, computing the time to the beginning of the next period
      		long int timetowait= 1000*((next_arrival_time[3].tv_sec - ora.tv_sec)*1000000+(next_arrival_time[3].tv_usec-ora.tv_usec));
      		if (timetowait < 0)
		{
			missed_deadlines[3]++;
		}
      		waittime.tv_sec = timetowait/1000000000;
      		waittime.tv_nsec = timetowait%1000000000;
   	   	nanosleep(&waittime, NULL);		// suspending the task until the beginning of the next period
      		// the task is ready: setting the next arrival time for the task (i.e., the beginning of the next period)
      		long int periods_micro=periods[3]/1000;
      		next_arrival_time[3].tv_sec = next_arrival_time[3].tv_sec + periods_micro/1000000;
      		next_arrival_time[3].tv_usec = next_arrival_time[3].tv_usec + periods_micro%1000000;
	}
	completion_flag[2]=true;
}

//thread code for pollingServer (used only for temporization)
void *pollingServer( void *ptr)
{
  	int i=0;
  	struct timespec waittime;
  	waittime.tv_sec=0;
  	waittime.tv_nsec = periods[0];
	while (!((completion_flag[0]) & (completion_flag[1]) & (completion_flag[2])))
	{
      		// executing application specific code
		pollingServer_code();
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
	printf("\n\nTask Execution Completed\n\nExiting Scheduler\n\n\n");
}

