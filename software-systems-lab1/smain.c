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
        if(is_pipeline(line)){
            char *commands[MAX_ARGS];
            int commandsc = 0;
            split_pipeline(line, commands, &commandsc);
            if(commandsc == 0){
                continue;
            } 
            else {
            exec_pipeline(commands, &commandsc);
            }
        } else 
        {
            parse_command(line, args, &argsc);
            //empty input handling
            if (argsc == 0) 
            { 
                continue;
            }  
            else if (is_exit(args)) 
            { 
                printf("exiting shell...\n");
                exit(0);
            } else if (is_cd(args)) 
            {
                cd(args, argsc); 
            } 
            else if(is_redirect(args, argsc)) 
            { 
            launch_program_with_redirection(args, argsc);
            }
            else { //Basic command 
                launch_program(args, argsc);
        }
        }
       
    }
    return 0;
}