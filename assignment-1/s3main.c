#include "s3.h"

// Process a single command line (handles pipelines, subshells, and basic commands)
static void process_input(char line[], char* prog_name) {

    char *args[MAX_ARGS];
    int argsc;

    // 1. Check for Pipeline
    if(is_pipeline(line)){
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
    // 2. Check for Subshell (MUST be separate from pipeline check)
    else if (is_subshell(line)) {
        launch_subshell(line, prog_name);
    }   
    // 3. Normal Command
    else 
    {
        parse_command(line, args, &argsc);
        //empty input handling
        if (argsc == 0) return;
        
        if (is_exit(args)) 
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

    char line[MAX_LINE];

    // --- MODE 1: Non-Interactive (Subshell Mode) ---
    // Usage: ./s3 -c "command string"
    if (argc == 3 && strcmp(argv[1], "-c") == 0) {
        strcpy(line, argv[2]);
        
        // Handle batching (;) inside the -c command string
        // Note: No while(1) loop here! Run once and exit.
        if(is_batch(line)){
            char *sub_lines[MAX_ARGS];
            int sub_linesc = 0;
            split_batch(line, sub_lines, &sub_linesc);
            for(int i = 0; i < sub_linesc; i++){
                process_input(sub_lines[i], argv[0]);
            }
        } else {
            process_input(line, argv[0]);
        }
        return 0; // EXIT IMMEDIATELY after running the command
    }

    // --- MODE 2: Interactive Mode (Normal Shell) ---
    // Usage: ./s3
    while (1) {
        read_command_line(line);
        if(is_batch(line)){
            char *sub_lines[MAX_ARGS];
            int sub_linesc = 0;
            split_batch(line, sub_lines, &sub_linesc);
            for(int i = 0; i < sub_linesc; i++){
                process_input(sub_lines[i], argv[0]);
            }
        } else {
            process_input(line, argv[0]);
        }
    }
    
    return 0;
}