#ifndef _S3_H_
#define _S3_H_

///See reference for what these libraries provide
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

///Constants for array sizes, defined for clarity and code readability
#define MAX_LINE 1024
#define MAX_ARGS 128
#define MAX_PROMPT_LEN 256

///Enum for readable argument indices (use where required)
enum ArgIndex
{
    ARG_PROGNAME,
    ARG_1,
    ARG_2,
    ARG_3,
};

///With inline functions, the compiler replaces the function call 
///with the actual function code;
///inline improves speed and readability; meant for short functions (a few lines).
///the static here avoids linker errors from multiple definitions (needed with inline).
static inline void reap()
{
    wait(NULL);
}

///Shell I/O and related functions (add more as appropriate)
void read_command_line(char line[]);
void construct_shell_prompt(char shell_prompt[]);
void parse_command(char line[], char *args[], int *argsc);
void exec_redirect(char *args[], int argsc);
void split_batch(char line[], char *commands[], int *commandsc);
void split_pipeline(char line[], char *commands[], int *commandsc);
void exec_pipeline(char *commands[], int *commandsc);

//flags for exit, cd and redirect
bool is_pipeline(char line[]);
bool is_batch(char line[]);
bool is_exit(char *args[]);
bool is_cd(char *args[]);
bool is_redirect(char *args[], int argsc);

void catch_fd_errors(int fd);

///Child functions (add more as appropriate)
void child(char *args[], int argsc);
void cd(char *args[], int argsc);

///Program launching functions (add more as appropriate)
void launch_program(char *args[], int argsc);
void launch_program_with_redirection(char *args[], int argsc);
#endif