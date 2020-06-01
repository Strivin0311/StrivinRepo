#include "lib.h"
#include "types.h"

int main(void) {
	// TODO in lab4
	// init sems
	sem_t mutex, full,empty;
	sem_init(&mutex,1);
	sem_init(&full,0);
	sem_init(&empty,6);

	for(int i=0;i<4;i++)
	{
		int ret = fork();
		if(!ret)  // child
			break;
		else if(ret == -1) // fork failed
			exit();
		else  // father
			continue;
	}

	int pid = getpid();
	// father as consumer
	if(pid == 1)
	{
		while(1)
		{
			sem_wait(&full);  // fullbuffers->P()
			sem_wait(&mutex); // mutex->P()
			printf("Consumer : consume\n"); // remove c from buffer
			sleep(128);
			sem_post(&mutex); // mutex->V()
			sem_post(&empty); // emptybuffers->V()
		}
	}

	// children as producers
	else if(pid<=5)
	{
		while(1)
		{
			sem_wait(&empty);  // emptybuffers->P()
			sem_wait(&mutex); // mutex->P()
			printf("Producer %d: produce\n", pid); // add c to buffer
			sleep(128);
			sem_post(&mutex); // mutex->V()
			sem_post(&full); // fullbuffers->V()
		}
	}
	
	// destroy sems
	sem_destroy(&mutex);
	sem_destroy(&full);
	sem_destroy(&empty);
	exit();
	return 0;
}
