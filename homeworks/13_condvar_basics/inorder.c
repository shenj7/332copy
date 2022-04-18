#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

int current_thread = 1;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

void *thread(void *arg)
{
        int *num = (int *)arg;
        pthread_mutex_lock(&lock);
        
        while (current_thread != *num) {
          pthread_cond_wait(&cond, &lock);
        }


        printf("%d wants to enter the critical section\n", *num);

        current_thread++;

        printf("%d is finished with the critical section\n", *num);
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&lock);

        return NULL;
}

int
main(int argc, char **argv)
{
        pthread_t threads[4];
        int i;
        int nums[] = {2, 1, 4, 3};

        for(i = 0; i < 4; ++i) {
                pthread_create(&threads[i], NULL, thread, &nums[i]);

                if(i == 2) {
                        sleep(3);
                }
        }

        for(i = 0; i < 4; ++i) {
                pthread_join(threads[i], NULL);
        }

        printf("Everything finished\n");

        return 0;
}
