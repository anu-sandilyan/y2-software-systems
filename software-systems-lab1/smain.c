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
    //stores exit, command, redirect flags
    bool exit_cmd; 
    bool cd_cmd;
    bool redirect_cmd;

    while (1) {
        //parsing alters line, so redirect check is done beforehand
        read_command_line(line);
        redirect_cmd = is_redirect(line);

        parse_command(line, args, &argsc);
        exit_cmd = is_exit(args);
        cd_cmd = is_cd(args);
        
        //empty input handling
        if (argsc == 0) { 
            continue;
        }  
        else if (exit_cmd) { 
            printf("exiting shell...\n");
            exit(0);
        } else if (cd_cmd) {
            cd(args, argsc); 
            reap(); 
        } else if(redirect_cmd) { 
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