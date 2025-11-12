#include "s3.h"

///Simple for now, but will be expanded in a following section
void construct_shell_prompt(char shell_prompt[])
{
    // make buffer: getcwb() requires a size buffer to prevent overflow
    char cwd_buffer[MAX_PROMPT_LEN - 10];
    if (getcwd(cwd_buffer, sizeof(cwd_buffer)) != NULL) {
        snprintf(shell_prompt, MAX_PROMPT_LEN, "%s[s3]$ ", cwd_buffer);
    } else {
        // default prompt if cwd fails
        perror("error: getcwd failed");
        strcpy(shell_prompt, "[s3]$ ");
    }
}

///Prints a shell prompt and reads input from the user
void read_command_line(char line[])
{
    char shell_prompt[MAX_PROMPT_LEN];
    construct_shell_prompt(shell_prompt);
    printf("%s", shell_prompt);

    ///See man page of fgets(...)
    if (fgets(line, MAX_LINE, stdin) == NULL)
    {
        perror("fgets failed");
        exit(1);
    }
    ///Remove newline (enter)
    line[strlen(line) - 1] = '\0';
}

void parse_command(char line[], char *args[], int *argsc)
{
    ///Implements simple tokenization (space delimited)
    ///Note: strtok puts '\0' (null) characters within the existing storage, 
    ///to split it into logical cstrings.
    ///There is no dynamic allocation.

    ///See the man page of strtok(...)
    char *token = strtok(line, " ");
    *argsc = 0;
    while (token != NULL && *argsc < MAX_ARGS - 1)
    {
        args[(*argsc)++] = token;
        token = strtok(NULL, " ");
    }
    ///args must be null terminated
    args[*argsc] = NULL; 
}

bool is_exit(char *args[]){
    if(strcmp(args[0], "exit") == 0){
        return true;
    } 
    else {
        return false;
        }
}

bool is_cd(char *args[]){
    if(strcmp(args[0], "cd") == 0){
        return true;
    } 
    else {
        return false;
        }
}

bool is_redirect(char *args[], int argsc){
    if(argsc > 0){
        for(int i = 0; i < argsc; i++){
            if(strcmp(args[i], ">>") == 0 || strcmp(args[i], ">") == 0 || strcmp(args[i], "<") == 0){
                return true;
            }
        } 
    } 
    return false;
}

/* bool is_pipeline(char *args[], int argsc){
    if(argsc > 0){
        for(int i = 0; i <argsc; i++){
            if(strcmp(args[i], "|") == 0){
                return true;
            }
        }
    }
    return false;
} */


///Launch related functions
void child(char *args[], int argsc)
{
        execvp(args[0], args);
    ///Use execvp to load the binary 
    ///of the command specified in args[ARG_PROGNAME].
        perror("execvp failed");
        exit(1);
}


void cd(char *args[], int argsc){ //cd function 
    if(argsc < 2){ //if no arg, cd to home
        chdir(getenv("HOME")); 
    } else {// normal case
        if(chdir(args[1]) != 0) {  //error handling
            perror("error: cd failed: invalid file or directory path");
        }
    }
}

void catch_fd_errors(int fd){ //handle file opening errors
    if(fd < 0){
        perror("error: file permission denied\n");
        exit(1);
    }
}


void launch_program_with_redirection(char *args[], int argsc){ 
    
    char *redirect_op = NULL; //input or output operator
    char *redirect_file = NULL; //filename
    int op_index = -1; //operator index in arguments

    for (int i = 0; i < argsc; i++){
        if(strcmp(args[i], ">") == 0 || strcmp(args[i], ">>") == 0 || strcmp(args[i], "<") == 0){
            redirect_op = args[i];
            op_index = i;

            if (i + 1 < argsc) {
                redirect_file = args[i + 1]; //file name should be next argument after operator
            } else {
                fprintf(stderr, "error: invalid redirection syntax: no filename provided\n");
                return; //return to shell
            }
            break;
        } 
    }

    if (op_index == -1 || redirect_file == NULL) { //error handling
         fprintf(stderr, "error: invalid redirection syntax\n");
         return;
    }

        int rc = fork();
        if (rc < 0) {
            fprintf(stderr, "fork failed :(\n");
            exit(1);
        } else if (rc == 0){
            printf("child initiated: pid = %d\n", getpid());
            int fd;
            //implement redirection
            if(strcmp(redirect_op, ">" )== 0){
                fd = open(redirect_file, O_WRONLY | O_CREAT | O_TRUNC, 0664); //open w write permissions, create if non-existent, truncate (overwrite)
                catch_fd_errors(fd);
                dup2(fd, STDOUT_FILENO); //redirect std output to fd
            } 
            else if(strcmp(redirect_op, ">>" )== 0){
                fd = open(redirect_file, O_WRONLY | O_CREAT | O_APPEND, 0664); //append instead of overwrite
                catch_fd_errors(fd);
                dup2(fd, STDOUT_FILENO); 
            }
            else if(strcmp(redirect_op, "<") == 0){
                fd = open(redirect_file, O_RDONLY); //write permissions not needed
                catch_fd_errors(fd);
                dup2(fd, STDIN_FILENO); //redirect std input to fd this time
            }
            close(fd); //cleanup
            args[op_index] = NULL; //program can't automatically process redirect operators
            child(args, argsc);
        } else {
            int wstatus;
            waitpid(rc, &wstatus, 0);
            printf("returning to parent function (pid =  %d): \n", getpid());
        }
}


void launch_program(char *args[], int argsc)
{

    int rc = fork();
    if (rc < 0) {
        fprintf(stderr, "fork failed :(\n");
        exit(1);
    } else if (rc == 0){
        printf("child initiated: pid = %d\n", getpid());
        child(args, argsc);
    } else {
        int wstatus;
        waitpid(rc, &wstatus, 0);
        printf("returning to parent function (pid =  %d): \n", getpid());
    }
    ///fork() a child process.
    ///In the child part of the code,
    ///call child(args, argv)
}