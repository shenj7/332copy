/* Copyright 2016 Rose-Hulman
   But based on idea from http://cnds.eecs.jacobs-university.de/courses/caoslab-2007/
   */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>


int main() {
    char command[82];
    char *parsed_command[2];
    //takes at most two input arguments
    // infinite loop but ^C quits
    while (1) {
        printf("SHELL%% ");
        fgets(command, 82, stdin);
        command[strlen(command) - 1] = '\0';//remove the \n
        int len_1;
        for(len_1 = 0;command[len_1] != '\0';len_1++){
            if(command[len_1] == ' ')
                break;
        }
        parsed_command[0] = command;
        if(len_1 == strlen(command)){
            printf("Command is '%s' with no arguments\n", parsed_command[0]); 
            parsed_command[1] = NULL;
        }else{
            command[len_1] = '\0';
            parsed_command[1] = command + len_1 + 1;
            printf("Command is '%s' with argument '%s'\n", parsed_command[0], parsed_command[1]); 
        }

        //rename cmd and argument for simplicity
        char* cmd = parsed_command[0];
        char* arg = parsed_command[1];

        //exit
        if (strncmp(cmd, ":q", 2) == 0) {
          printf("exiting\n");
          return 0;
        }

        //fork the process so one can run shell while the other runs the command
        int pid = fork();
        if (pid == 0) {
          printf("background %d\n", strncmp(cmd, "BG", 2));
          if (strncmp(cmd, "BG", 2) == 0) {
            //background processes
            //printf("running background process\n");
            char* bgcmd = malloc(strlen(&command[2]));
            strncpy(bgcmd, &command[2], strlen(&command[2])); //use command for string operations
            //printf("background command: %s\n", bgcmd);
            int bgpid = fork();
            if (bgpid == 0) {
              //printf("bgps %s start!!!\n", bgcmd);
              execlp(bgcmd, bgcmd, arg, NULL);
              exit(1);
            }
            free(bgcmd);
            int bgstatus;
            wait(&bgstatus);
            printf("background process completed\n");
            exit(2);
            printf("background process executed with status %d\n", WEXITSTATUS(bgstatus));
          } else {
            printf("running foreground process\n");
            execlp(cmd, cmd, arg, NULL);
            exit(3);
            //foreground processes
          }
        }
        int status;
        wait(&status);
        //printf("foreground process executed with status %d\n", WEXITSTATUS(status));
    }
}
