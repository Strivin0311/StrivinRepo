#include "lib.h"
#include "types.h"

int main(void) {
	// TODO in lab4
	// init forks
	sem_t forks[5];
	sem_init(&forks[0],1);
	sem_init(&forks[1],1);
	sem_init(&forks[2],1);
	sem_init(&forks[3],1);
	sem_init(&forks[4],1);

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
    // think and eat
	while(1)
	{
		printf("Philosopher %d: think\n", pid);  // think
		sleep(128);
 		if(pid%2==0)
		{
 			sem_wait(&forks[pid]);         //P(fork[i]);
 			sem_wait(&forks[(pid+1)%5]);  //P(fork[(i+1)%N]);
 		} 
		else
		{
 			sem_wait(&forks[(pid+1)%5]);  //P(fork[(i+1)%N]);
 			sem_wait(&forks[pid]);         //P(fork[i]);
 		}
 		printf("Philosopher %d: eat\n", pid); //eat
		sleep(128);
 		sem_post(&forks[pid]);         //V(fork[i]);
 		sem_post(&forks[(pid+1)%5]);  //V(fork[(i+1)%N]);
	}
	
	// destroy forks
	sem_destroy(&forks[0]);
	sem_destroy(&forks[1]);
	sem_destroy(&forks[2]);
	sem_destroy(&forks[3]);
	sem_destroy(&forks[4]);
	exit();
	return 0;
}
