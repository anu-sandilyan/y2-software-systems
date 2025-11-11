#include "s3.h"

int main(int argc, char *argv[]){

    ///Stores the command line input
    char line[MAX_LINE];
    char line_copy[MAX_LINE];

    ///Stores pointers to command arguments.
    ///The first element of the array is the command name.
    char *args[MAX_ARGS];

    ///Stores the number of arguments
    int argsc;

    while (1) {

        read_command_line(line);
        strcpy(line_copy, line); // parse modifies line, so must make a copy for strcmp
        parse_command(line, args, &argsc); //change parse to top of loop 
        
        if (argsc == 0) { //no input
            continue;
        } else if (strcmp(args[0], "exit") == 0) { // exit command
            printf("exiting shell...\n");
            exit(0);
        } else if (strcmp(args[0], "cd") == 0) { //cd function
            cd(args, argsc); 
            reap(); 
        } else if(strstr(line, ">") != NULL || strstr(line_copy, "<") != NULL){ //Command with redirection
           launch_program_with_redirection(args, argsc);
           reap();
       }
       else { //Basic command 
           launch_program(args, argsc);
           reap();
       }
       
    }
    return 0;
}