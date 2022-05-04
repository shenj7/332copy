#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/mman.h>
#include <unistd.h>
#include "forth/forth_embed.h"

// if the region requested is already mapped, things fail
// so we want address that won't get used as the program
// starts up
#define STACKHEAP_MEM_START 0xf9f8c000

// the number of memory pages will will allocate to an instance of forth
#define NUM_PAGES 20

// the max number of pages we want in memort at once, ideally
#define MAX_PAGES 3

int priorities[NUM_PAGES]; // 3, 2, 1, (0 means unmapped with file) (-1 means unmapped)
int fds[NUM_PAGES];

int currentpage = 0;

unsigned int pagemem = 0xf9f8c000;

static void unmap() {
    for (int i = 0; i < NUM_PAGES; i++) {
        if (priorities[i] == 1) {

            printf("unmapping page %d\n", i);
            munmap((void*)STACKHEAP_MEM_START+i*getpagesize(), getpagesize()); // unmap pages with 0 priority
        }
        // printf("%d", priorities[i]);
        if (priorities[i] >= 1) {
            priorities[i]--;
        }
    }
}

static void handler(int sig, siginfo_t *si, void *unused)
{
    void* fault_address = si->si_addr;



    printf("in handler with invalid address %p\n", fault_address);
    int distance = fault_address - (void*) STACKHEAP_MEM_START;
    unmap();

    int page = distance/getpagesize();
    // printf("mapping page %d, priority %d\n", page, priorities[page]);
    printf("mapping page %d\n", page);

    if (priorities[page] == -1) { // not yet mapped to file
        char filename[11];
        sprintf(filename, "page_%d.dat", page);
        // printf("creating file %s\n", filename);
        int fd = open(filename, O_RDWR | O_CREAT, S_IRWXU);
        if(fd < 0) {
            perror("error loading linked file");
            exit(25);
        }
        char data = '\0';
        lseek(fd, getpagesize() - 1, SEEK_SET);
        write(fd, &data, 1);
        lseek(fd, 0, SEEK_SET);
        fds[page] = fd;
        // void* result = mmap((void*)STACKHEAP_MEM_START + page*getpagesize(),
        //                 getpagesize(),
        //                 PROT_READ | PROT_WRITE | PROT_EXEC,
        //                 MAP_FIXED | MAP_SHARED,
        //                 -1,
        //                 0);

        char* result = mmap((void*)STACKHEAP_MEM_START + page*getpagesize(),
                            getpagesize(),
                            PROT_READ | PROT_WRITE | PROT_EXEC,
                            MAP_FIXED | MAP_SHARED,
                            fds[page], 0);
        if(result == MAP_FAILED) {
            perror("map failed");
            exit(1);
        }
    } else { // retrieve from file
        char* result = mmap((void*)STACKHEAP_MEM_START + page*getpagesize(),
                            getpagesize(),
                            PROT_READ | PROT_WRITE | PROT_EXEC,
                            MAP_FIXED | MAP_SHARED,
                            fds[page], 0);
        if(result == MAP_FAILED) {
            perror("map failed");
            exit(1);
        }
    }
    priorities[page] = MAX_PAGES;
}

int main() {
    for (int i = 0; i < NUM_PAGES; i++) {
        priorities[i] = -1; // initialize
    }
    struct forth_data forth;
    char output[200];

    static char stack[SIGSTKSZ];
    
    stack_t ss = {
                  .ss_size = SIGSTKSZ,
                  .ss_sp = stack,
    };
    sigaltstack(&ss, NULL);
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = handler;
    if (sigaction(SIGSEGV, &sa, NULL) == -1) {
        perror("error installing handler");
        exit(3);
    }
    int returnstack_size = getpagesize() * 2;
    void* returnstack = mmap(NULL, returnstack_size, PROT_READ | PROT_WRITE | PROT_EXEC,
                   MAP_ANON | MAP_PRIVATE, -1, 0);
    int stackheap_size = getpagesize() * NUM_PAGES;
    void* stackheap = (void*) STACKHEAP_MEM_START;
    
    initialize_forth_data(&forth,
                          returnstack + returnstack_size, //beginning of returnstack
                          stackheap, //begining of heap
                          stackheap + stackheap_size); //beginning of stack
    load_starter_forth_at_path(&forth, "forth/jonesforth.f");
    printf("finished loading starter forth\n");
    int fresult = f_run(&forth,
                        " : USESTACK BEGIN DUP 1- DUP 0= UNTIL ; " // function that puts numbers 0 to n on the stack
                        " : DROPUNTIL BEGIN DUP ROT = UNTIL ; " // funtion that pulls numbers off the stack till it finds target
                        " FOO 5000 USESTACK " // 5000 integers on the stack
                        " 2500 DROPUNTIL " // pull half off
                        " 1000 USESTACK " // then add some more back
                        " 4999 DROPUNTIL " // pull all but 2 off
                        " . . " // 4999 and 5000 should be the only ones remaining, print them out
                        " .\" finished successfully \" " // print some text */
                        ,
                        output,
                        sizeof(output));
    if(fresult != FCONTINUE_INPUT_DONE) {
        printf("forth did not finish executing sucessfully %d\n", fresult);
        exit(4);
    }
    printf("OUTPUT: %s\n", output);    
    printf("done\n");
    return 0;
}
