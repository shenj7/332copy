#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <string.h>

#define simple_assert(message, test) \
    do                               \
    {                                \
        if (!(test))                 \
            return message;          \
    } while (0)
#define TEST_PASSED NULL
#define DATA_SIZE 100
#define INITIAL_VALUE 77
#define MAX_TESTS 10

char *(*test_funcs[MAX_TESTS])(); // array of function pointers that store
                                  // all of the tests we want to run
int num_tests = 0;

int data[DATA_SIZE][DATA_SIZE]; // shared data that the tests use

int fpip[MAX_TESTS][2];

void add_test(char *(*test_func)())
{
    if (num_tests == MAX_TESTS)
    {
        printf("exceeded max possible tests");
        exit(1);
    }
    test_funcs[num_tests] = test_func;
    num_tests++;
}
// this setup function should run before each test
void setup()
{
    printf("starting setup\n");
    for (int i = 0; i < DATA_SIZE; i++)
    {
        for (int j = 0; j < DATA_SIZE; j++)
        {
            data[i][j] = INITIAL_VALUE;
        }
    }
    // imagine this function does a lot of other complicated setup
    // that takes a long time
    usleep(3000000);
    // printf("finished setup\n");
}

char *test1();
char *test2();
char *test3();

char *test1()
{

    printf("starting test 1\n");
    for (int i = 0; i < DATA_SIZE; i++)
    {
        for (int j = 0; j < DATA_SIZE; j++)
        {
            simple_assert("test 1 data not initialized properly", data[i][j] == INITIAL_VALUE);
        }
    }

    for (int i = 0; i < DATA_SIZE; i++)
    {
        for (int j = 0; j < DATA_SIZE; j++)
        {
            data[i][j] = 1;
        }
    }

    for (int i = 0; i < DATA_SIZE; i++)
    {
        for (int j = 0; j < DATA_SIZE; j++)
        {
            simple_assert("test 1 data not set properly", data[i][j] == 1);
        }
    }
    printf("ending test 1\n");
    return TEST_PASSED;
}

char *test2()
{

    printf("starting test 2\n");
    for (int i = 0; i < DATA_SIZE; i++)
    {
        for (int j = 0; j < DATA_SIZE; j++)
        {
            simple_assert("test 2 data not initialized properly", data[i][j] == INITIAL_VALUE);
        }
    }

    for (int i = 0; i < DATA_SIZE; i++)
    {
        for (int j = 0; j < DATA_SIZE; j++)
        {
            data[i][j] = 2;
        }
    }

    for (int i = 0; i < DATA_SIZE; i++)
    {
        for (int j = 0; j < DATA_SIZE; j++)
        {
            simple_assert("test 2 data not set properly", data[i][j] == 2);
        }
    }

    printf("ending test 2\n");
    return TEST_PASSED;
}

char *test3()
{

    printf("starting test 3\n");

    simple_assert("test 3 always fails", 1 == 2);

    printf("ending test 3\n");
    return TEST_PASSED;
}

char *test4()
{

    printf("starting test 4\n");

    int *val = NULL;
    printf("data at val is %d", *val);

    printf("ending test 4\n");
    return TEST_PASSED;
}

char *test5()
{

    printf("starting test 5\n");

    while (1)
    {
    }

    printf("ending test 5\n");
    return TEST_PASSED;
}

void run_test(char *(fn)(), int i)
{
    alarm(3);
    char *res = fn();
    if (res == TEST_PASSED)
    {
        close(fpip[i][1]);
        exit(0);
        // printf("Test Passed\n");
    }
    else
    {
        write(fpip[i][1], res, strlen(res));
        // printf("res: %s", res);
        close(fpip[i][1]);
        exit(1);
        // printf("Test Failed: %s\n",res);
    }
}

void run_all_tests()
{

    //    pthread_t tests[100];
    //
    //
    //    for(int i = 0; i < num_tests; i++) {
    //        if(pthread_create(&tests[i], NULL, &run_test, test_funcs[i])) {
    //
    //            printf("Error creating thread\n");
    //            exit(2);
    //
    //        }
    //    }
    //    for(int i = 0; i < num_tests; i++) {
    //        char* result = NULL;
    //        if(pthread_join(tests[i],(void**) &result)) {
    //            printf("Error joining thread\n");
    //            exit(2);
    //        }
    //        if(result == TEST_PASSED) {
    //            printf("Test Passed\n");
    //        } else {
    //            printf("Test Failed: %s\n",result);
    //        }
    //
    //    }

    pid_t pids[num_tests];
    setup();
    for (int i = 0; i < num_tests; i++)
    {
        pids[i] = fork();
        if (pids[i] == 0)
        {
            run_test(test_funcs[i], i);
            printf("test done\n");
        }
    }
    for (int i = 0; i < num_tests; i++)
    {
        int stat;
        waitpid(pids[i], &stat, 0);
        if (!WIFEXITED(stat))
        {
            printf("test crashed\n");
        }
        else
        {
            if (WEXITSTATUS(stat) == 0)
            {
                printf("test passed\n");
            }
            else if (WEXITSTATUS(stat) == 1)
            {
                int recv_len = 1;
                char buff[128];
                read(fpip[i][0], buff, 128);
                printf("test failed: %s\n", buff);
            }
            else if (WEXITSTATUS(stat) == 2)
            {
                printf("test timed out\n");
            }
            else
            {
                printf("something went wrong\n");
            }
        }
    }
}

void catch_alarm()
{
    exit(2);
}

void setpipes()
{
    for (int i = 0; i < num_tests; i++)
    {
        if (pipe(fpip[i]) == -1)
        {
            perror("pipe failed\n");
        }
    }
}

void main()
{
    signal(SIGALRM, catch_alarm);
    add_test(test1);
    add_test(test2);
    add_test(test3);
    add_test(test4); // uncomment for Step 4
    add_test(test5); // uncomment for Step 5
    setpipes();
    run_all_tests();
}
