# Y2 Software Systems Assignments
##### By Anu Sandilyan and Divyaa Nagasundhar

## Table of Contents
- #### Assignment 1 - Shell Implementation
    - Compilation
    - Features Implemented
    - Proposed Extensions (PEs)
    - Usage Examples
    - Implementation Details
- #### Assignment 2 - Multithreaded Chat Application
    - Compilation
    - Implemented Requests
    - User interface
    - Proposed Extensions (PEs)
        - History at Connection
        - Remove Inactive Clients
    - Future Plans


## Assignment 1 - Shell Implementation

The **s3** program is a functional shell implementation in C, designed to replicate fundamental features of standard shells like Bash. This project demonstrates process management, inter-process communication (pipes), file redirection, and recursive command execution.

### Compilation

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

### Features Implemented

#### 1. Basic Command Execution

The shell parses user input using a string tokenizer, and executes standard shell commands (e.g., ls, grep, echo) using fork(), execvp(), and wait() calls to handle child process creation, execution and reaping.

#### 2. Built-in Commands

Added special functions to parse and execute these more complex commands:

* exit: Terminates the shell session.

* cd: Changes the current working directory.

* cd (no args): Changes to the user's HOME directory.

* cd -: Toggles to the previous working directory.

* cd <path>: Changes to the specified path.

Additionally, the shell prompt dynamically updates to show the current working directory: [/current/path][s3]$.

#### 3. Redirection

Supports standard input/output redirection using dup2():

* \> : Redirect standard output to a file (overwrite existing content).

* \>> : Redirect standard output to a file (append to existing content).

* < : Redirect standard input from a file.

#### 4. Pipelines

Supports linking multiple commands where the output of one becomes the input of the next using pipe(), and supports multiple pipes in one input e.g.
```bash
ls | grep .c | wc -l
```

#### 5. Batched Commands
Supports executing multiple independent commands on a single line separated by semicolons (;), e.g
```bash
mkdir test; cd test; ls
```

### Proposed Extensions (PEs)
#### PE 1: Subshells

The shell supports grouping commands inside parentheses ( ... ). These commands execute in a separate 'subshell' child process. Side effects (like cd) inside the subshell do not affect the main shell. This was implemented by recursively launching the shell binary with the -c flag.

#### PE 2: Nested Subshells
The shell supports nesting subshells to arbitrary depths.

Example: (echo Level 1 ; (echo Level 2 ; (cd /tmp ; pwd)))

Logic: The parser identifies the outer parentheses, spawns a child shell, which then parses the inner content, recursively spawning further children if necessary.

### Implementation Details

* Process Management: The shell uses a standard implementation of parent-child processes. The parent waits for the child process to complete unless pipelining is involved, in which case it manages file descriptors to chain processes together.

* Tokenization: User input is parsed using strtok for simple commands, but custom parsing logic is used for subshells and batching to handle nested delimiters safely.

* Subshell Recursion: When a subshell (...) is detected, the program calls fork() and the child executes execvp("./s3", "-c", "inner_command"). This re-uses the shell's own parsing logic for the inner content.




## Assignment 2 - Multithreaded Chat Application

The **chat_server.c** program implements a multi client chat server, that manages all clients and processes incoming commands. The **chat_client.c** program recieves and sends the commands to the server, and communicates with the user. Both programs communicate with each other through the **udp.h** file thats acts as a wrapper functions to open a UDP socket 


### Compilation

To compile the shell, ensure `chat_server.c`, `chat_client.c`, and `udp.h` are in the same directory. Open the Linux terminal and navigate to the directory with the program files and type:

```bash
pkill -9 -x chat_client
pkill -9 -x chat_server
```
To ensure there are no running threads that may cause unpredicatable and dangerous behaviour. It should just be run once each on one of the many terminals.
Open as many terminals as you want clients (+1 one to behave as the server) and type:
```bash
gcc chat_client.c -o chat_client -lncursesw -pthread
```
to compile the chat_client program and connect it to the lncurses and pthread library, 
and then run:
```bash
gcc chat_server.c -o chat_server
```
to compile the chat_client program.
Then, type:
```bash
./chat_server &
```
to run the chat server program.
 And then on only the remaining terminals connecting as clients, type:
```bash
./chat_client
```
to begin running the client-server program!

### Implemented Requests

We made the client-server system compatible with 8 main commands:

```bash
conn$ client_name
```
where client_name can be replaced with whatever name the user wants ( no spaces allowed ) as long as it is not already taken by another client. A check is in place to ensure multiple clients do not try connect to the server through the same server.
```bash
say$ msg
```
where msg can be replaced with whatever message the user wants
```bash
sayto$ recipient_name msg
```
where recipient_name can be replaced with the name of the intended client to the user wants to privately message and msg can be replaced with whatever message the user wants to send
```bash
mute$ client_name
```
where client_name can be replaced with the name of the intended client to the user wants to mute which will then prevent them for seeing any messages sent from that client. Only works if that client is already unmuted.
```bash
unmute$ client_name
```
where client_name can be replaced with the name of the intended client to the user wants to unmute which will then allow them to see any messages sent from that client again. Only works if that client is already muted.
```bash
rename$ new_name
```
where new_name can be replaced with whatever name the user wants replace their name with as long as it is not already taken by another client
```bash
disconn$
```
where the user can disconnect themselves from the serve
```bash
kick$ client_name
```
where the admin ( connected on port 6666 ) can forcefully disconnect a client from the server


### User interface


### Proposed Extensions (PEs)

#### 1. History at Connection


#### 2. Remove Inactive Clients
The time stamp for each client is logged when they last submitted a request. Every 30 seconds, the time since the last request is calculated and checked to see if it is longer than 5 minutes. If so, a ping is sent to the client to let the user know they have been inactive for an extended period of time in case it was unintentional inactvity. The user can then interact through another request within 30s ( before the next activity check ) or send a **ret-ping$** request that wont output anything but just let the server know the user is still there without having to interact with the other clients if they do not wish to.

### Future plans - if given more time
