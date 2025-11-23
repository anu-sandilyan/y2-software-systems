#include "s3.h"

//new function to process each line, or "sub-line" if user inputs a batched command with ";"
static void process_input(char line[]){

    ///Stores pointers to command arguments.
    ///The first element of the array is the command name.
    char *args[MAX_ARGS];

    ///Stores the number of arguments
    int argsc;

    if(is_pipeline(line)){
        //split pipeline into commands
        char *commands[MAX_ARGS];
        int commandsc = 0;
        split_pipeline(line, commands, &commandsc);
        if(commandsc == 0){
            return;
        } 
        else {
        exec_pipeline(commands, &commandsc);
        }
    }   
    //parse normally if no pipeline is found
    else 
    {
        parse_command(line, args, &argsc);
        //empty input handling
        if (argsc == 0) 
        { 
            return;
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

int main(int argc, char *argv[]){

    ///Stores the command line input
    char line[MAX_LINE];

    ///Stores pointers to command arguments.
    ///The first element of the array is the command name.
    char *args[MAX_ARGS];

    ///Stores the number of arguments
    int argsc;

    while (1) {
        read_command_line(line);
        if(is_batch(line)){
            char *sub_lines[MAX_ARGS];
            int sub_linesc = 0;
            split_batch(line, sub_lines, &sub_linesc);
            for(int i = 0; i < sub_linesc; i++){
                process_input(sub_lines[i]);
            }
        } else {
            process_input(line);
        }
       
    }
    return 0;
}