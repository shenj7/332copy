#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/types.h>


#define simple_assert(message, test) do { if (!(test)) return message; } while (0)
#define TEST_PASSED NULL
#define DATA_SIZE 100
#define INITIAL_VALUE 77
#define MAX_TESTS 10

char* (*test_funcs[MAX_TESTS])(); // array of function pointers that store
                           // all of the tests we want to run
int num_tests = 0;

int data[DATA_SIZE][DATA_SIZE]; // shared data that the tests use

void add_test(char* (*test_func)()) {
    if(num_tests == MAX_TESTS) {
        printf("exceeded max possible tests");
        exit(1);
    }
    test_funcs[num_tests] = test_func;
    num_tests++;
}
// this setup function should run before each test
void setup() {
    printf("starting setup\n");
    for(int i = 0; i < DATA_SIZE; i++) {
        for(int j = 0; j < DATA_SIZE; j++) {
            data[i][j] = INITIAL_VALUE;
        }
    }
    // imagine this function does a lot of other complicated setup
    // that takes a long time
    usleep(3000000);
    printf("finished setup\n");
}

char* test1();
char* test2();
char* test3();


char* test1() {

    printf("starting test 1\n");
    for(int i = 0; i < DATA_SIZE; i++) {
        for(int j = 0; j < DATA_SIZE; j++) {
            simple_assert("test 1 data not initialized properly", data[i][j] == INITIAL_VALUE);
        }
    }

    for(int i = 0; i < DATA_SIZE; i++) {
        for(int j = 0; j < DATA_SIZE; j++) {
            data[i][j] = 1;
        }
    }
    
    for(int i = 0; i < DATA_SIZE; i++) {
        for(int j = 0; j < DATA_SIZE; j++) {
            simple_assert("test 1 data not set properly", data[i][j] == 1);
        }
    }
    printf("ending test 1\n");
    return TEST_PASSED;
}

char* test2() {

    printf("starting test 2\n");
    for(int i = 0; i < DATA_SIZE; i++) {
        for(int j = 0; j < DATA_SIZE; j++) {
            simple_assert("test 2 data not initialized properly", data[i][j] == INITIAL_VALUE);
        }
    }

    for(int i = 0; i < DATA_SIZE; i++) {
        for(int j = 0; j < DATA_SIZE; j++) {
            data[i][j] = 2;
        }
    }

    for(int i = 0; i < DATA_SIZE; i++) {
        for(int j = 0; j < DATA_SIZE; j++) {
            simple_assert("test 2 data not set properly", data[i][j] == 2);
        }
    }

    printf("ending test 2\n");
    return TEST_PASSED;
}

char* test3() {

    printf("starting test 3\n");

    simple_assert("test 3 always fails", 1 == 2);
    
    printf("ending test 3\n");
    return TEST_PASSED;
}


char* test4() {

    printf("starting test 4\n");

    int *val = NULL;
    printf("data at val is %d", *val);
    
    printf("ending test 4\n");
    return TEST_PASSED;
}

char* test5() {

    printf("starting test 5\n");

    while(1) { } 
    
    printf("ending test 5\n");
    return TEST_PASSED;
}

void run_test(char* (fn)()) {
  setup();
  int pid = fork();
  if (pid == 0) {
    char* res = fn();
    printf("result: %s\n", res);
    exit(0);
  }
  int stat;
  wait(&stat);
  printf("test done\n");
}


void run_all_tests() {

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

  
  int setuppid = fork();
  int setupstatus;
  if (setuppid == 0) {
    setup();
  } else {
    wait(&setupstatus);
    printf("setup: %d", data[0][0]);
    for (int i = 0; i < num_tests; i++) {
      run_test(test_funcs[i]);
    }    
  }
}

void main() {
    add_test(test1);
    add_test(test2);
    add_test(test3);
    // add_test(test4); // uncomment for Step 4
    // add_test(test5); // uncomment for Step 5
    run_all_tests();
    
    printf("more setup: %d%d", data[0][0], data[1][1]);
}
