#include "s3.h"

///Simple for now, but will be expanded in a following section
void construct_shell_prompt(char shell_prompt[])
{
    //  cwd_buffer: getcwd() requires a size buffer to prevent overflow
    char prompt_buffer[MAX_PROMPT_LEN - 10];
    if (getcwd(prompt_buffer, sizeof(prompt_buffer)) != NULL) {
        snprintf(shell_prompt, MAX_PROMPT_LEN, "%s[s3]$ ", prompt_buffer);
    } else {
        // default prompt if getcwd fails
        perror("error: getcwd failed");
        strcpy(shell_prompt, "[s3]$ ");
    }
}

//Prints a shell prompt and reads input from the user
void read_command_line(char line[])
{
    char shell_prompt[MAX_PROMPT_LEN];
    construct_shell_prompt(shell_prompt);
    printf("%s", shell_prompt);

    ///See man page of fgets(...)
    if (fgets(line, MAX_LINE, stdin) == NULL) //get user input
    {
        perror("fgets failed");
        exit(1);
    }
    ///Remove newline (enter)
    line[strlen(line) - 1] = '\0';
}

/*potential extension: add support for  \n and input strings in quotations?
 currently doesn't work as this function only checks space as a delimiter*/
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

void split_pipeline(char line[], char *commands[], int *commandsc)
{
    ///splits user input into seperate commands, delimited by "|" character
    char *token = strtok(line, "|");
    *commandsc = 0;
    while (token != NULL && *commandsc < MAX_ARGS - 1)
    {
        commands[(*commandsc)++] = token;
        token = strtok(NULL, "|");
    }
    ///args must be null terminated
    commands[*commandsc] = NULL; 
}

void split_batch(char line[], char *sub_lines[], int *sub_linesc)
{
    ///splits user input into  seperate inputs, delimited by ";" character
    char *token = strtok(line, ";");
    *sub_linesc = 0;
    while (token != NULL && *sub_linesc < MAX_ARGS - 1)
    {
        sub_lines[(*sub_linesc)++] = token;
        token = strtok(NULL, ";");
    }
    ///args must be null terminated
    sub_lines[*sub_linesc] = NULL; 
}

void exec_pipeline(char *commands[], int *commandsc) {

    //initialise pipe with 2 descriptors (read and write)
    int prev_pipe_read = -1; 
    int pd[2]; //

    //loop to process pipeline for each command
    for (int i = 0; i < *commandsc; i++) {
        if (i < *commandsc - 1) { //do not create another pipe for last command
            if (pipe(pd) < 0) {
                perror("pipe failed");
                exit(1);
            }
        }
        //create child process for each command
        int rc = fork();
        if (rc < 0) {
            perror("fork failed");
            exit(1);
        } else if (rc == 0) {
            //child process
            if (prev_pipe_read != -1) {
                dup2(prev_pipe_read, STDIN_FILENO); //wire read end of pipe to input
                close(prev_pipe_read);
            }
            if (i < *commandsc - 1) {
                dup2(pd[1], STDOUT_FILENO); //wire write end of pipe to output
                close(pd[1]); // close child copy of pipe
                close(pd[0]);
            }

            if (is_subshell(commands[i])) {
                // launch_subshell forks internally, so we wait for it
                launch_subshell(commands[i], "./s3"); 
                exit(0); // Exit this pipeline child when the subshell finishes
            }

            char *args[MAX_ARGS];
            int argsc;
            parse_command(commands[i], args, &argsc);

            if (argsc == 0) {
                exit(0);
            }

            if (is_redirect(args, argsc)) {
                exec_redirect(args, argsc);
            } 
            execvp(args[0], args); // (slide 23)
            perror("execvp failed");
            exit(1);
        } 
        // close parent copy of pipe
        if (prev_pipe_read != -1) {
            close(prev_pipe_read);
        }
        if (i < *commandsc - 1) {
            prev_pipe_read = pd[0];
            close(pd[1]); 
        }
    }

    for (int i = 0; i < *commandsc; i++) {
        wait(NULL);
    }
}

void exec_redirect(char *args[], int argsc)
{
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
                exit(1); //return to shell
            }
            break;
        } 
    }

    if (op_index == -1 || redirect_file == NULL) 
    { //error handling
         fprintf(stderr, "error: invalid redirection syntax\n");
         exit(1);
    }

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
    
}

bool is_exit(char *args[])
{
    if(strcmp(args[0], "exit") == 0){
        return true;
    } 
    else {
        return false;
        }
}

bool is_cd(char *args[])
{
    if(strcmp(args[0], "cd") == 0){
        return true;
    } 
    else {
        return false;
        }
}

bool is_redirect(char *args[], int argsc)
{
    if(argsc > 0){
        for(int i = 0; i < argsc; i++){
            if(strcmp(args[i], ">>") == 0 || strcmp(args[i], ">") == 0 || strcmp(args[i], "<") == 0){
                return true;
            }
        } 
    } 
    return false;
}

bool is_pipeline(char line[])
{
    if(strstr(line, "|") != NULL){
        return true;
    }
    return false;
} 

bool is_batch(char line[])
{
    if(strstr(line, ";") != NULL){
        return true;
    }
    return false;
} 

bool is_subshell(char line[]) {
    char *start = line;
    while (*start == ' ' || *start == '\t') start++;

    // Check if it starts with '('
    if (*start != '(') return false;

    // Check if it ends with ')'
    int len = strlen(line);
    char *end = line + len - 1;
    while (end > start && (*end == ' ' || *end == '\t')) end--;

    if (*end == ')') return true;
    
    return false;
}

void launch_subshell(char line[], char *prog_name) {
    // 1. Strip the parentheses to get the inner command
    char *start = line;
    while (*start != '(') start++; // Find first '('
    start++; // Move past '('

    // Find last ')'
    char *end = line + strlen(line) - 1;
    while (*end != ')') end--;
    *end = '\0'; // Replace ')' with null terminator to cut the string

    // 2. Fork and Execute the shell recursively
    int rc = fork();
    if (rc < 0) {
        perror("fork failed");
        exit(1);
    } else if (rc == 0) {
        // Child Process: run ./s3 -c "command string"
        char *args[] = {prog_name, "-c", start, NULL};
        execvp(prog_name, args);
        
        perror("subshell execution failed");
        exit(1);
    } else {
        wait(NULL);
    }
}

///Launch related functions
void child(char *args[], int argsc)
{
        execvp(args[0], args);
    ///Use execvp to load the binary 
    ///of the command specified in args[ARG_PROGNAME].
        perror("execvp failed");
        exit(1);
}

//TO DO: add functionality for special cd commands (e.g. ~, -)
void cd(char *args[], int argsc)
{ 
    static char old_pwd[MAX_PROMPT_LEN] = "";
    char current_pwd[MAX_PROMPT_LEN];

    // 1. Get current directory so we can save it later as 'old_pwd'
    if (getcwd(current_pwd, sizeof(current_pwd)) == NULL) {
        perror("error: getcwd failed");
        return; 
    }

    char *new_dir = NULL;
    bool print_new_dir = false;

    // 2. Determine the target directory based on arguments
    if(argsc < 2) { 
        // Case: "cd" (no arguments) -> Go to HOME
        new_dir = getenv("HOME");
    } 
    else if (strcmp(args[1], "~") == 0) {
        // Case: "cd ~" -> Go to HOME
        new_dir = getenv("HOME");
    } 
    else if(strcmp(args[1], "-") == 0){
        // Case: "cd -" -> Go to previous directory
        if (old_pwd[0] == '\0') {
            fprintf(stderr, "error: cd failed: OLDPWD not set\n");
            return;
        }
        new_dir = old_pwd;
        print_new_dir = true;
    } 
    else { 
        // Case: "cd foldername" -> Go to that folder
        new_dir = args[1];
    }

    // 3. Safety check
    if (new_dir == NULL) {
         fprintf(stderr, "error: cd failed: invalid destination\n");
         return;
    }

    // 4. Execute chdir ONLY ONCE using the resolved 'new_dir'
    // (Do not use args[1] here, as it might be "~" or "-")
    if (chdir(new_dir) != 0) {
        perror("error: cd failed");
    } else {
        // 5. Success! Update old_pwd
        strcpy(old_pwd, current_pwd);
        
        // Print directory if user used "cd -"
        if (print_new_dir) {
            printf("%s\n", new_dir);
        }
    }
}

void catch_fd_errors(int fd)
{ //handle file opening errors
    if(fd < 0){
        perror("error: file permission denied\n");
        exit(1);
    }
}

void launch_program_with_redirection(char *args[], int argsc)
{ 
        int rc = fork();
        if (rc < 0) {
            fprintf(stderr, "fork failed :(\n");
            exit(1);
        } else if (rc == 0){
            exec_redirect(args, argsc);
            child(args, argsc);
        } else {
            int wstatus;
            waitpid(rc, &wstatus, 0);
        }
}


void launch_program(char *args[], int argsc)
{
    int rc = fork();
    if (rc < 0) {
        fprintf(stderr, "fork failed :(\n");
        exit(1);
    } else if (rc == 0){
        child(args, argsc);
    } else {
        int wstatus;
        waitpid(rc, &wstatus, 0);
    }
    ///fork() a child process.
    ///In the child part of the code,
    ///call child(args, argv)
}
