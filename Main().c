#include "sch-helpers.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

/** Declaring some global variables and structs **/
int numberOfProcesses;
int nextProcess;
int totalWaitingTime;
int totalContextSwitches;
int simulationTime;
int cpuTimeUtilized;
int timeQuantum;
int preReadyQueueSize;
process processes[MAX_PROCESSES + 1];
process* preReadyQueue[MAX_PROCESSES];
process_queue readyQueue;
process_queue waitingQueue;
process* cpus[NUMBER_OF_PROCESSORS];


int compareProcessPointers(const void* aa, const void* bb)
{
	process* a = *((process**)aa);
	process* b = *((process**)bb);
	if (a->pid < b->pid) return -1;
	if (a->pid > b->pid) return 1;
	assert(0);
	return 0;

}
int compareProcessPriority(const void* aa, const void* bb)
{
	process* a = *((process**)aa);
	process* b = *((process**)bb);
	if (a->priority < b->priority) return -1;
	if (a->priority > b->priority) return 1;
	assert(0);
	return 0;

}

int runningProcesses(void)
{
	int result = 0;
	int i;
	for (i = 0; i < NUMBER_OF_PROCESSORS; i++)
	{
		if (cpus[i] != NULL) result++;

	}
	return result;
}

int incomingProcesses(void)
{
	return numberOfProcesses - nextProcess;
}

process* nextScheduledProcess(void)
{
	if (readyQueue.size == 0) return NULL;
	process* result = readyQueue.front->data;
	dequeueProcess(&readyQueue);
	return result;
}

void moveIncomingProcessesRR(void)
{
	while (nextProcess < numberOfProcesses && processes[nextProcess].arrivalTime <= simulationTime)
	{
		processes[nextProcess].quantumRemaining = timeQuantum;
		preReadyQueue[preReadyQueueSize++] = &processes[nextProcess++];
	}
}
void moveIncomingProcessesPR(void)
{
	while (nextProcess < numberOfProcesses && processes[nextProcess].arrivalTime <= simulationTime)
	{
		preReadyQueue[preReadyQueueSize++] = &processes[nextProcess++];
	}
}

void moveWaitingProcesses(void)
{
	int i;
	int size = waitingQueue.size;
	
	for (i = 0; i < size; i++)
	{
		process* front = waitingQueue.front->data;
		dequeueProcess(&waitingQueue);

		assert(front->bursts[front->currentBurst].step <= front->bursts[front->currentBurst].length);

		if (front->bursts[front->currentBurst].step == front->bursts[front->currentBurst].length)
		{
			front->currentBurst++;
			front->quantumRemaining = timeQuantum;
			preReadyQueue[preReadyQueueSize++] = front;
		}
		else 
		{
			enqueueProcess(&waitingQueue, front);
		}
	}
}

void moveReadyProcessesRR(void)
{
	int i;
	qsort(preReadyQueue, preReadyQueueSize, sizeof(process*),	compareProcessPointers);

	for (i = 0; i < preReadyQueueSize; i++)
	{
		enqueueProcess(&readyQueue, preReadyQueue[i]);
	}
	preReadyQueueSize = 0;

	for (i = 0; i < NUMBER_OF_PROCESSORS; i++)
	{
		if (cpus[i] == NULL)
		{
			cpus[i] = nextScheduledProcess();
		}
	}
}

void moveReadyProcessesPR(void)
{
	int i;
	
	qsort(preReadyQueue, preReadyQueueSize, sizeof(process*), compareProcessPriority);
	for (i = 0; i < preReadyQueueSize; i++)
	{
		enqueueProcess(&readyQueue, preReadyQueue[i]);
	}
	preReadyQueueSize = 0;

	for (i = 0; i < NUMBER_OF_PROCESSORS; i++)
	{
		if (cpus[i] == NULL)
		{
			cpus[i] = nextScheduledProcess();
		}
	}
}

void moveRunningProcessesRR(void)
{
	int i;
	for (i = 0; i < NUMBER_OF_PROCESSORS; i++)
	{
		if (cpus[i] != NULL) 
		{
			if (cpus[i]->bursts[cpus[i]->currentBurst].step == cpus[i]->bursts[cpus[i]->currentBurst].length)
			{
				cpus[i]->currentBurst++;
				if (cpus[i]->currentBurst < cpus[i]->numberOfBursts)
				{
					enqueueProcess(&waitingQueue, cpus[i]);
				}
				else
				{
					cpus[i]->endTime = simulationTime;
				}
				cpus[i] = NULL;
			}
			else if (cpus[i]->quantumRemaining == 0)
			{
				cpus[i]->quantumRemaining = timeQuantum;
				preReadyQueue[preReadyQueueSize++] = cpus[i];
				totalContextSwitches++;
				cpus[i] = NULL;
			}
		}
	}
}

void moveRunningProcessesPR(void)
{
	int i;
	for (i = 0; i < NUMBER_OF_PROCESSORS; i++)
	{
		if (cpus[i] != NULL)
		{
			if (cpus[i]->bursts[cpus[i]->currentBurst].step ==
				cpus[i]->bursts[cpus[i]->currentBurst].length)
				{
				cpus[i]->currentBurst++;
				if (cpus[i]->currentBurst < cpus[i]->numberOfBursts)
				{
					enqueueProcess(&waitingQueue, cpus[i]);
				}
				else
				{
					cpus[i]->endTime = simulationTime;
				}
				cpus[i] = NULL;
			}
		}
	}
}

void updateWaitingProcesses(void)
{
	if (waitingQueue.front != NULL)
	{
		process* front = waitingQueue.front->data;
		if (front != NULL)
		{
			front->bursts[front->currentBurst].step++;
		}
	}
}

void updateReadyProcesses(void)
{
	int i;
	for (i = 0; i < readyQueue.size; i++)
	{
		process* front = readyQueue.front->data;
		dequeueProcess(&readyQueue);
		front->waitingTime++;
		enqueueProcess(&readyQueue, front);
	}
}

void updateRunningProcessesRR(void)
{
	int i;
	for (i = 0; i < NUMBER_OF_PROCESSORS; i++)
	{
		if (cpus[i] != NULL)
		{
			cpus[i]->bursts[cpus[i]->currentBurst].step++;
			cpus[i]->quantumRemaining--;
		}
	}
}

void updateRunningProcessesPR(void)
{
	int i;
	for (i = 0; i < NUMBER_OF_PROCESSORS; i++)
	{
		if (cpus[i] != NULL)
		{
			cpus[i]->bursts[cpus[i]->currentBurst].step++;
		}
	}
}

void initializeGlobals(void)
{
	int i = 0;
	for (; i < NUMBER_OF_PROCESSORS; i++)
	{
		cpus[i] = NULL;
	}
	simulationTime = 0;
	cpuTimeUtilized = 0;
	totalWaitingTime = 0;
	totalContextSwitches = 0;
	numberOfProcesses = 0;
	nextProcess = 0;
	preReadyQueueSize = 0;

	initializeProcessQueue(&readyQueue);
	initializeProcessQueue(&waitingQueue);
}

/*-----------------------------------MAIN()-----------------------------------*/

int main(int argc, char * argv[])
{
	int sumOfTurnaroundTimes = 0;
	int doneReading = 0;
	int i;
	int schedulerType = 0;
	FILE* fptr;
	
	if (strcmp(argv[1], "RR") == 0)
	{
		timeQuantum = atoi(argv[2]);
		schedulerType = 1;
		fptr = fopen(argv[3], "r");
	}
	
	else if (strcmp(argv[1], "PR") == 0)
	{
		schedulerType = 2;
		fptr = fopen(argv[2], "r");
	}
	
	else 
	{
		fptr = NULL;
	}
	
	initializeGlobals();
	
	while (doneReading = readProcess(&processes[numberOfProcesses], fptr))
	{
		if (doneReading == 1) numberOfProcesses++;
		if (numberOfProcesses > MAX_PROCESSES) break;
	}

	if (numberOfProcesses == 0)
	{
		fprintf(stderr, "[Error]: no processes specified in input.\n");
		return -1;
	}
	
	else if (numberOfProcesses > MAX_PROCESSES)
	{
		fprintf(stderr, "[Error]: too many processes specified in input; they cannot be more than: %d.\n", MAX_PROCESSES);
		return -1;
	}

	qsort(processes, numberOfProcesses, sizeof(process), compareByArrival);

	if (schedulerType == 1)
	{
		while (1) {
			moveIncomingProcessesRR();
			moveRunningProcessesRR();
			moveWaitingProcesses();
			moveReadyProcessesRR();

			updateWaitingProcesses();
			updateReadyProcesses();
			updateRunningProcessesRR();

			cpuTimeUtilized += runningProcesses();
			
			if (runningProcesses() == 0 && incomingProcesses() == 0 && waitingQueue.size == 0) 
				break;

			simulationTime++;
		}
	}
	
	else if (schedulerType == 2)
	{
		while (1) {
			moveIncomingProcessesPR();
			moveRunningProcessesPR();
			moveWaitingProcesses();
			moveReadyProcessesRR();

			updateWaitingProcesses();
			updateReadyProcesses();
			updateRunningProcessesPR();

			cpuTimeUtilized += runningProcesses();

			if (runningProcesses() == 0 && incomingProcesses() == 0 && waitingQueue.size == 0) 
				break;

			simulationTime++;
		}
	}
	
	for (i = 0; i < numberOfProcesses; i++)
	{
		sumOfTurnaroundTimes += processes[i].endTime - processes[i].arrivalTime;
		totalWaitingTime += processes[i].waitingTime;
	}
	
	if (schedulerType == 1)
	{
		printf("Input File Name\t : %s\n"
			"CPU Scheduling Alg\t : %s(%d)\n"
			"Average CPU utilization\t : %.1f%%\n"
			"Average turnaround time\t: %.2f units\n"
			"Average Response time\t : %.2f units\n",
			argv[3],
			argv[1],
			timeQuantum,
			100.0 * cpuTimeUtilized / simulationTime,
			sumOfTurnaroundTimes / (double)numberOfProcesses,
			totalWaitingTime / (double)numberOfProcesses);
	}
	else if (schedulerType == 2)
	{
		printf("Input File Name\t : %s\n"
			"CPU Scheduling Alg\t : %s\n"
			"Average CPU utilization\t : %.1f%%\n"
			"Average turnaround time\t : %.2f units\n"
			"Average Response time\t : %.2f units\n",
			argv[2],
			argv[1],
			timeQuantum,
			100.0 * cpuTimeUtilized / simulationTime,
			sumOfTurnaroundTimes / (double)numberOfProcesses,
			totalWaitingTime / (double)numberOfProcesses);
	}
	
	printf("\n");
	return 0;
}
