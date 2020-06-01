#include "lib.h"
#include "types.h"

int main(void) {
	// TODO in lab4
	// init mutex
	sem_t writemutex,countmutex;
	sem_init(&writemutex,1);
	sem_init(&countmutex,1);
	int Rcount = 0 ;

	for(int i=0;i<5;i++)
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
	// writer
	if(pid<=3)
	{
		while(1)
		{
			sem_wait(&writemutex);//P(WriteMutex);
			printf("Writer %d: write\n", pid);//write; 
			sleep(128);
			sem_post(&writemutex);//V(WriteMutex);
		}
	}

	// reader
	else if(pid<=6)
	{
		while(1)
		{
			sem_wait(&countmutex);//P(CountMutex);
			read(SH_MEM, (uint8_t *)&Rcount, 4,0);
			if (Rcount == 0)
 				sem_wait(&writemutex);//P(WriteMutex);
			++Rcount; 
			write(SH_MEM, (uint8_t *)&Rcount, 4,0);
			sem_post(&countmutex);//V(CountMutex);
			printf("Reader %d: read, total %d reader\n", pid, Rcount);//read; 
			sleep(128);
			// after read
			sem_wait(&countmutex);//P(CountMutex);
			read(SH_MEM, (uint8_t *)&Rcount, 4,0);
			--Rcount;
			write(SH_MEM, (uint8_t *)&Rcount, 4,0);
			if (Rcount == 0)
 				sem_post(&writemutex);//V(WriteMutex);
			sleep(128);
			sem_post(&countmutex);//V(CountMutex);
		}
	}

	// destroy mutexs
	sem_destroy(&writemutex);
	sem_destroy(&countmutex);
	exit();
	return 0;
}
