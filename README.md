# Assignment 1 - Shell Implementation & Assignment 2 - Multithreaded Chat Application

## Table of Contents
- #### Assignment 1 - Shell Implementation
    - [Compilation](#compilation)
    - [Features Implemented](#features-implemented)
    - [Proposed Extensions (PEs)](#proposed-extensions-pes)
    - [Usage Examples](#usage-examples)
    - [Implementation Details](#implementation-details)
- #### Assignment 2 - Multithreaded Chat Application

### Assignment 1 - Shell Implementation

The **s3** program is a functional shell implementation in C, designed to replicate fundamental features of standard shells like Bash. This project demonstrates process management, inter-process communication (pipes), file redirection, and recursive command execution.

## Compilation

To compile the shell, ensure `s3.c`, `s3main.c`, and `s3.h` are in the same directory. Open the Linux terminal and navigate to the directory with the program files. Use the GNU Compiler Collection (GCC) and type:

```bash
gcc s3.c s3main.c -o s3
```
To execute the s3 shell program, type:
```bash
./s3
```
If the file does not have sufficient permissions to execute, run the command:
```bash
chmod +x s3
```
Then rerun the previous command, and it should start.

## Features Implemented

### 1. Basic Command Execution

The shell parses user input using a string tokenizer, and executes standard shell commands (e.g., ls, grep, echo) using fork(), execvp(), and wait() calls to handle child process creation, execution and reaping.

### 2. Built-in Commands

Added special functions to parse and execute these more complex commands:

* exit: Terminates the shell session.

* cd: Changes the current working directory.

* cd (no args): Changes to the user's HOME directory.

* cd -: Toggles to the previous working directory.

* cd <path>: Changes to the specified path.

Additionally, the shell prompt dynamically updates to show the current working directory: [/current/path][s3]$.

### 3. Redirection

Supports standard input/output redirection using dup2():

* \> : Redirect standard output to a file (overwrite existing content).

* \>> : Redirect standard output to a file (append to existing content).

* < : Redirect standard input from a file.

### 4. Pipelines

Supports linking multiple commands where the output of one becomes the input of the next using pipe(), and supports multiple pipes in one input e.g.
```bash
ls | grep .c | wc -l
```

### 5. Batched Commands
Supports executing multiple independent commands on a single line separated by semicolons (;), e.g
```bash
mkdir test; cd test; ls
```

## Proposed Extensions (PEs)
### PE 1: Subshells

The shell supports grouping commands inside parentheses ( ... ). These commands execute in a separate 'subshell' child process. Side effects (like cd) inside the subshell do not affect the main shell. This was implemented by recursively launching the shell binary with the -c flag.

### PE 2: Nested Subshells
The shell supports nesting subshells to arbitrary depths.

Example: (echo Level 1 ; (echo Level 2 ; (cd /tmp ; pwd)))

Logic: The parser identifies the outer parentheses, spawns a child shell, which then parses the inner content, recursively spawning further children if necessary.

## Implementation Details

* Process Management: The shell uses a standard implementation of parent-child processes. The parent waits for the child process to complete unless pipelining is involved, in which case it manages file descriptors to chain processes together.

* Tokenization: User input is parsed using strtok for simple commands, but custom parsing logic is used for subshells and batching to handle nested delimiters safely.

* Subshell Recursion: When a subshell (...) is detected, the program calls fork() and the child executes execvp("./s3", "-c", "inner_command"). This re-uses the shell's own parsing logic for the inner content.
