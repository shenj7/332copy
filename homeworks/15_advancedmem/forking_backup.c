#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include "forth/forth_embed.h"
#include "forking_forth.h"

// if the region requested is already mapped, things fail
// so we want address that won't get used as the program
// starts up
#define UNIVERSAL_PAGE_START 0xf9f8c000

// the number of memory pages will will allocate to an instance of forth
#define NUM_PAGES 22 // last two pages are for the return stack
#define MAX_FORTHS 10

// this is a function I define for you - it's at the bottom of the
// file if you're curious
void push_onto_forth_stack(struct forth_data *data, int64_t value_to_push);

int ufd; // do i need this?

int frames[NUM_PAGES*MAX_FORTHS]; //0 = unused, 1 = in use
int page_table[NUM_PAGES];
int forks[MAX_FORTHS];
#define PAGE_UNCREATED -1
#define CREATED 1

struct forth_extra_data {
    bool valid;
    struct forth_data data;
    int page_table[NUM_PAGES];
};

static void handler(int sig, siginfo_t *si, void *unused);

struct forth_extra_data forth_extra_data[MAX_FORTHS];  

int uforthnum = 0;

int fork_forth();

void switch_current_to(int forthnum);

int used_pages_count;

int get_used_pages_count() {
      used_pages_count = 0;

      for(int i = 0; i < NUM_PAGES*MAX_FORTHS; i++){
        if(frames[i] != PAGE_UNCREATED){
                      used_pages_count++;        
        }
            
      }
    return used_pages_count;
}


bool first_time = true;
void initialize_forths() {
    if(first_time) {
        // here's the place for code you only want to run once, like registering
        // our SEGV signal handler

        first_time = false;
               
        char filename[11];
        sprintf(filename, "bigdat.dat");
        int fd = open(filename, O_RDWR | O_CREAT, S_IRWXU);
        if(fd < 0) {
            perror("error loading linked file");
            exit(25);
        }
        char data = '\0';
        lseek(fd, getpagesize()*MAX_FORTHS*NUM_PAGES, SEEK_SET);
        write(fd, &data, 1);
        lseek(fd, 0, SEEK_SET);
    //void* result = mmap(NULL,
    //    MAX_FORTHS*NUM_PAGES*getpagesize(),
    //    PROT_READ | PROT_WRITE | PROT_EXEC,
    //    MAP_SHARED,
    //    fd, 0);
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

      ufd = fd;//do i need this?
    }
    // here's the place for code you want to run every time we run a test case
    for (int i = 0; i < MAX_FORTHS*NUM_PAGES; i++) {
      frames[i] = PAGE_UNCREATED;
    }

    // mark all the forths as invalid
    for(int i = 0; i < MAX_FORTHS; i++) {
        forth_extra_data[i].valid = false;
        forks[i] = PAGE_UNCREATED;
        for (int j = 0; j < NUM_PAGES; j++) {
        forth_extra_data[i].page_table[j] = PAGE_UNCREATED;
        }
    }
    
    used_pages_count = 0;

}

int find_available_slot() {
    int forth_num;
    for(forth_num = 0; forth_num < MAX_FORTHS; forth_num++) {
        if(forth_extra_data[forth_num].valid == false) {
            break; // we've found a num to use
        }
    }
    if(forth_num == MAX_FORTHS) {
        printf("We've created too many forths!");
        exit(1);
    }
    return forth_num;
}

// This function creates a brand new forth instance (not a fork) with the given code
// The function returns the id num of the newly created forth
int create_forth(char* code) {
    int forth_num = find_available_slot();
    forth_extra_data[forth_num].valid = true;


    // STEP 0
    // this is where you should allocate NUM_PAGES*getpagesize() bytes
    // starting at position UNIVERSAL_PAGE_START to get started
    //
    // use mmap

    //void* result = mmap((void*) UNIVERSAL_PAGE_START,
    //    NUM_PAGES*getpagesize(),
    //    PROT_READ | PROT_WRITE | PROT_EXEC,
    //    MAP_FIXED|MAP_SHARED|MAP_ANONYMOUS,
    //    -1, 0);
    switch_current_to(forth_num);
    // the return stack is a forth-specific data structure.  I
    // allocate a seperate space for it as the last 2 pages of
    // NUM_PAGES.
    int returnstack_size = getpagesize() * 2;

    int stackheap_size = getpagesize() * (NUM_PAGES - 2);

    // note that in this system, to make forking possible, all forths
    // are created with pointers only in the universal memory region
    initialize_forth_data(&forth_extra_data[forth_num].data,
                          (void*) UNIVERSAL_PAGE_START + stackheap_size + returnstack_size, //beginning of returnstack
                          (void*) UNIVERSAL_PAGE_START, //begining of heap
                          (void*) UNIVERSAL_PAGE_START + stackheap_size); //beginning of the stack


    load_starter_forth_at_path(&forth_extra_data[forth_num].data, "forth/jonesforth.f");

    char output[100], input[100];

    // creating the fork function using FCONTINUE_FORK so we don't have to hard-code its value
    snprintf(input, 100, ": FORK %d PAUSE_WITH_CODE ;", FCONTINUE_FORK);

    // add a super tiny bit of forth which adds the FORK function
    f_run(&forth_extra_data[forth_num].data, input, output, 100);

         
    forth_extra_data[forth_num].data.input_current = code;
    return forth_num;
}

struct run_output run_forth_until_event(int forth_to_run) {
    struct run_output output;
    switch_current_to(forth_to_run);
    output.result_code = f_run(&forth_extra_data[forth_to_run].data,
                               NULL,
                               output.output,
                               sizeof(output.output));
    output.forked_child_id = -1; // this should only be set to a value if we are forking
    if(output.result_code == FCONTINUE_FORK) {
        printf("fork not yet implemented\n");
        // output.forked_child_id = fork_forth(forth_to_run);
    } 
    return output;

}

void push_onto_forth_stack(struct forth_data *data, int64_t value_to_push) {
    int64_t current_top = *((int32_t*) data->stack_top);
    *((int64_t*) data->stack_top) = value_to_push;
    data->stack_top -= 8; // stack is 8 bytes a entry, starts high,
                          // goes low
    *((int64_t*) data->stack_top) = current_top;
    
}

void switch_current_to(int forthnum){
  uforthnum = forthnum;
  forth_extra_data[forthnum].valid = true;
  munmap((void*) UNIVERSAL_PAGE_START,
      getpagesize()*NUM_PAGES*MAX_FORTHS);
  //mmap((void*) UNIVERSAL_PAGE_START, 
  //      getpagesize()*NUM_PAGES, 
  //      PROT_READ | PROT_WRITE | PROT_EXEC,
  //      MAP_FIXED | MAP_SHARED, 
  //      ufd, 
  //      forthnum*NUM_PAGES*getpagesize());
  for (int i = 0; i < NUM_PAGES; i++) {
    if (forth_extra_data[uforthnum].page_table[i] != PAGE_UNCREATED) {
      mmap((void*)UNIVERSAL_PAGE_START + i*NUM_PAGES*getpagesize(),
          getpagesize(),
          PROT_READ | PROT_WRITE | PROT_EXEC,
          MAP_FIXED | MAP_SHARED,
          ufd,
          forth_extra_data[forthnum].page_table[i]*getpagesize());
    }
    
  }
}


static void handler(int sig, siginfo_t *si, void *unused)
{
    void* fault_address = si->si_addr;
    int offset;
    int distance = (void*) fault_address - (void*) UNIVERSAL_PAGE_START;
    int page = distance/getpagesize();

    printf("in handler with invalid address %p\n", fault_address);

    offset = forth_extra_data[uforthnum].page_table[page];
    
    if (offset == PAGE_UNCREATED) {
      for (int i = 0; i < NUM_PAGES*MAX_FORTHS; i++) {
        printf("frame %d : %d\n", i, frames[i]);
        if (frames[i] == PAGE_UNCREATED) {
          forth_extra_data[uforthnum].page_table[page] = i;
          frames[i] = CREATED;
          offset = i;
          break;
        }
      }
    }

    printf("mapping page %d\n", page);
    if(distance < 0 || distance > (getpagesize() * NUM_PAGES)) {
             printf("address not within expected page!\n");
                          exit(2);
                          
    }

    
}

int fork_forth() {
  int loc = -1;

  for (int i = 0; i < MAX_FORTHS; i++) {
    if (forks[i] != PAGE_UNCREATED) {
      loc = i;
      forks[i] = CREATED;
      break;
    }
  }

  int pid = fork();
  if (pid == 0) {
    //child
    struct forth_data dat;
    int child_pages[NUM_PAGES];
    memcpy(&dat, &forth_extra_data[uforthnum].data, sizeof(struct forth_data)); // copy forth data into child
    for (int i = 0; i < NUM_PAGES*MAX_FORTHS; i++) {
      if (frames[i] == PAGE_UNCREATED) {
        forth_extra_data[forth_num_global].page_table[page] = i;
        frames[i] = CREATED;
        break;
      }
    }
  }
  int status;
  wait(&status);
  return pid;
}
