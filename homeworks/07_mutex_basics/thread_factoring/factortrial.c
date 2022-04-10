/* Copyright 2016 Rose-Hulman Institute of Technology

Here is some code that factors in a super dumb way.  We won't be
attempting to improve the algorithm in this case (though that would be
the correct thing to do).

Modify the code so that it starts the specified number of threads and
splits the computation among them.  You can be sure the max allowed 
number of threads is 50.  Be sure your threads actually run in parallel.

Your threads should each just print the factors they find, they don't
need to communicate the factors to the original thread.

ALSO - be sure to compile this code with -pthread or just used the 
Makefile provided.

 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
unsigned long long int target, i = 0;
int numThreads;

void *factor_runner(void* ptr) {
  unsigned long long int* start = (unsigned long long int*) ptr;
  printf("thread %llu started up to target %llu starting from %llu\n", *start, target, *start);
  for (unsigned long long int i = *start; i <= target/2; i = i + numThreads) {
    printf("thread %llu testing %llu\n", *start, i);
    if (target % i == 0) {
      printf("%llu is a factor\n", i);
    }
  }
  pthread_exit(NULL);
}


int main(void) {
  /* you can ignore the linter warning about this */
  printf("Give a number to factor.\n");
  scanf("%llu", &target);

  printf("How man threads should I create?\n");
  scanf("%d", &numThreads);
  if (numThreads > 50 || numThreads < 1) {
    printf("Bad number of threads!\n");
    return 0;
  }

  pthread_t threads[numThreads];
  int starts[numThreads];


  for (int i = 0; i < numThreads; i++) {
    // pthread_attr_t attr;
    // int rtn;
    // pthread_attr_init(&attr);
    // rtn = pthread_attr_setscope(&attr, PTHREAD_SCOPE_PROCESS);
    // printf("setscope: %d\n", rtn);
    // pthread_create(&threads[i], &attr, factor_runner, &i);
    starts[i] = i;
    pthread_create(&threads[i], NULL, factor_runner, &starts[i]);
  }
  for (int i = 0; i < numThreads; i++) {
    pthread_join(threads[i], NULL);
  }
  return 0;
}

