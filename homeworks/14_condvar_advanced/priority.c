#include <stdio.h>
#include <pthread.h>
#include <unistd.h>


/**
  In this system there are threads numbered 1-6 and a critical
  section.  The thread numbers are priorities, where thread 6 is
  highest priority, thread 5 is next, etc.

  If the critical section is empty, any thread can enter.  While a
  thread is in the critical section no other threads can enter -
  regardless of priority. When a thread leaves the critical section
  and there are threads waiting, the highest priority waiting thread
  is allowed to enter the critical section.

  Be sure a newly arriving thread can't jump into the critical
  section as the current thread finishes, bypassing priority rules.
  Solve this problem with mutexes/condition variables
 **/
#define max_threads 100
int currentind = 0;
int currentthreads[max_threads];
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

int initt() {
	for (int i = 0; i < max_threads; i++) {
		currentthreads[i] = 0;
	}
}

int findmax() {
	int max = 0;
	for (int i = 0; i < max_threads; i++) {
		if (currentthreads[i] > max) {
			max = currentthreads[i];
		}
	}
	return max;

}

void *thread(void *arg)
{
	int tindex = 0;
	int *num = (int *) arg;
	printf("%d wants to enter the critical section\n", *num);
	pthread_mutex_lock(&lock);
	tindex = currentind;
	currentthreads[currentind] = *num;
	currentind ++;
	pthread_mutex_unlock(&lock);
	while (findmax() != *num) {
		pthread_cond_wait(&cond, &lock);
	}
	pthread_mutex_unlock(&lock);

	pthread_mutex_lock(&lock);
	printf("%d has entered the critical section\n", *num);
	sleep(1);
	printf("%d is finished with the critical section\n", *num);
	currentthreads[tindex] = 0;
	pthread_mutex_unlock(&lock);
	pthread_cond_broadcast(&cond);

	return NULL;
}

int
main(int argc, char **argv)
{
	int i;
	pthread_t threads[6];
	int nums[] = {2, 1, 4, 3, 5, 6};
	initt();


	for(i = 0; i < 6; i++) {
		pthread_create(&threads[i], NULL, thread, &nums[i]);

		if(i == 2) sleep(10);
	}

	for(i = 0; i < 6; i++) {
		pthread_join(threads[i], NULL);
	}

	printf("Everything finished.\n");
}
