# Y2 Software Systems - Assignment 1 and 2
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
    - Features Implemented
    - Synchronisation
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

The **chat_server.c** program implements a multithreaded client-chat server, that manages all clients and processes incoming client requests. The **chat_client.c** program recieves and sends the commands to the server, and communicates with the user. The client and the server communicate with each other through the **udp.h** file thats acts as a wrapper functions to open a UDP socket.

### Compilation

To compile the shell, ensure `chat_server.c`, `chat_client.c`, and `udp.h` are in the same directory. Open the Linux terminal and navigate to the directory with the program files and type:

```bash
pkill -9 -x chat_client
pkill -9 -x chat_server
```
To ensure there are no running background processes that may cause unpredicatable and dangerous behaviour. To compile the client program, type:
```bash
gcc chat_client.c -o chat_client -lncursesw -pthread
```
to compile the chat server, run the command:
```bash
gcc chat_server.c -o chat_server -pthread
```
To start running the server, type:
```bash
./chat_server &
```
The '&' ensures it continuously runs as a background process. This is now the server terminal, where server-side messages such as client connection/disconnection, kicking and renaming updates will print.

 To connect a client, open a new terminal window and cd to the directory containing the client program. Then type:
```bash
./server &
./chat_client
```
to begin running the client-server chat program! Repeat this process in a new terminal window as many times as desired to connect more clients to the chat. 

### Features Implemented

We initially made the client-server system compatible with these 8 core functions:
1. Connect (conn$): Allows a new client to connect to the server, and prints a global message to the server and all clients.

```bash
conn$ client_name
```
* 'client_name' can be replaced with whatever name the user wants (no spaces are allowed) as long as it is not already taken by another client. The program also ensures that multiple clients cannot connect to the server with the same name. This was implemented by creating a global linked list of clients, and adding a new ClientNode for each new client that connects. This enabled us to easily traverse the list, fetch client attributes, and add/remove clients, forming the basis for writing more complex functions.

2. Global Messaging (say$): A client can send a global message, which is seen by every client in the server (unless another client has muted them).
```bash
say$ msg
```
* 'msg' is replaced with whatever message the client wants to send.

3. Private Messaging (sayto$): A client can send a private message to another user, which can only be seen by the sender and the recipient.

```bash
sayto$ recipient_name msg
```
* 'recipient_name' can be replaced with the name of the recipient that the sender wants to privately message, and 'msg' can be replaced with whatever message the user wants to send.

4. Muting (mute$): A client can stop seeing all messages (both global and private) from another user after muting them.
```bash
mute$ client_name
```
* 'client_name' can be replaced with the name of the client that the sender wants to mute, which will then prevent them from seeing any of that client's messages. This required the creation of another linked list, belonging to each client, of their own muted clients. During testing we encountered a bug that if a client renamed themeslves, they would no longer be muted to other clients - we fixed this by also updating the 'muted clients' list inside the rename function. We also added a check to alert the user if the client they were trying to mute was already muted. Only the sender recieves a notification.

5. Unmute(unmute$): A client can see messages again from a previously muted user.
```bash
unmute$ client_name
```
* 'client_name' can be replaced with the name of the client that the user wants to unmute, which allows them to see the previously muted user's messages again. This works similarly to the mute function, but removes the recipient from the sender's 'muted clients' list. This function also alerts the user if the client they were trying to unmute was already unmuted.  Only the sender recieves a notification.

6. Rename (rename$): A user can change their own name globally.
```bash
rename$ new_name
```
* 'new_name' can be replaced with the user's new name, as long as it is not taken by another client. This prints a global message to the server and all clients to alert that they have renamed themselves. 

7. Disconnect (disconn$): Allows a connected client to disconnect from the server, and prints a global message to the server and all clients.
```bash
disconn$
```
* No client name is needed, as the client can only use this command on themselves. This prints a goodbye message to the disconnecting client, and notifies the server and all other clients that they have left. It removes them from the global linked list of clients.

8. Kick (kick$): Allows an admin (client connected from port 6666) to kick any user from the server.
```bash
kick$ client_name
```
* For the scope of this assignment, an 'admin' connection was defined as a user connecting from port 6666. Whilst connecting as a 'normal' client automatically connects you to a random free port, the command:
```bash
./client [port_number]
```
* Allows the user to connect from any chosen port. If the client connects from port 6666, they are designated as an admin and can kick other users. If a non-admin types the kick$ command, they will see an error message and the server is notified of the unauthorised kick attempt. If a admin successfully kicks another user, the recipient sees a removal message, they are disconnected from the chat, and the server and other clients recieve a notification.

Whilst these functions provided much of the core functionality for our program, we wanted to enhance the user experience by implementing more functionality, as outlined in the proposed extensions. We also chose this point to add support for multithreading, which involved integrating locks with the pthread library - this is essential for a multi-client server, as many clients may be trying to perform read and write operations simultaneously, which could lead to data corruption and very slow server performance.

### Synchronisation

### User interface

Another program requirement was to implement a basic terminal UI, to separate the client input window and chat window. We decided to go beyond this, to create a fully functional and easy-to-read interface using the 'Ncurses' C library for terminal control. This design splits the terminal window into distinct sections: a scrolling chat history panel at the top and a fixed input bar at the bottom. This ensures that incoming messages do not disrupt the user's typing experience. We handled screen refreshing and cursor management manually in the chat_client program to maintain a clean layout even during rapid message updates. The terminal is split into 2 clear sections:

* Chat Window: A scrollable window occupying the top portion of the screen. I utilized the scrollok() and wsetscrreg() functions inNcurses to define a scrolling region that keeps the borders intact while allowing text to move.

* Input Window: A fixed 3-line panel at the bottom for user input. This separates messages from user input, preventing incoming messages from disrupting or overwriting the text the user is currently typing.

While this was sufficient for the project brief, we decided to further improve readability and overall UX by adding color-coding for different notifications, so the client could easily differentiate between them. This was done using the init_pair() function in Ncurses to add color pairs, with a check to only use this if the terminal supports colors, as older terminals may otherwise crash. The colors implemented are as follows:

* Green: New client connections
* Red: Error messages
* Grey: Client disconnections, kicking, muting and unmuting
* Blue: Renaming
* Yellow: Private messages
* White: default (global) messages and server-side notifications.

### Proposed Extensions (PEs)

#### 1. History at Connection
This feature enables a newly connected client to see previous messages and global notifications (such as connections or renames), up to a maximum of 15 messages. This involved the implementation of a circular buffer array (ChatHistory struct) which can hold 15 strings, and overwrites itself using modular arithmetic logic when its length is exceeded, This 'wrap-around' functionality allows the most recent 15 messages to be stored, allowing clients to catch up with the conversation immediately even if they have just connected.

#### 2. Remove Inactive Clients
The time stamp for each client is logged when they last submitted a request. Every 30 seconds, the time since the last request is calculated and checked by the server to see if it is longer than 5 minutes. If so, a ping is sent to the client to let the user know they have been inactive for an extended period of time, in case it was unintentional inactvity. The user can then interact through by sending any request within 30s (before the next activity check) or send a **ret-ping$** request that has no output, but lets the server know the user is still present (without having to interact with other clients if they do not wish to).

### Error Detection
During our thorough testing and debugging of each function, we added error detection for almost all incorrect or invalid commands that a client can type. Errors will display an informative error message to the client, which are outlined here:

* Invalid Command Format: If the client does not format their message as 'command$ msg'.
* Unknown Command: If the client uses the correct format, but tries to type a command that does not exist.
* Unauthorised Kick Attempt: If a non-admin user (connected from a port other than the 6666 admin port), tries to kick another user.
* User not found: If a client attempts to mute, unmute, kick or privately message a client that does not exist in the global list.
* Thread Create Failed: Should not ever print, but is in place in case thread creation fails.
* Invalid Username: If a client tries to connect with a username that has a space in it ( this would break other requests that operate by splitting the request at the specified spaces
* Already taken username: If a client tries to connect with or change their name to one that already exists
* Already connected: Trying to connect more than once to server from the same terminal

### Future plans

Whilst we feel like we have implemented the required functionality and extended the project brief thoroughly, we could improve the chat program even more in future. Possible extensions include allowing clients to reconnect with preserved history, which may involve adding a full database of previously connected clients. We also did not have sufficient time to fully explore the limitations of UDP, and with more time we could experiment switching to TCP which may have more accurate message delivery.

Overall, we feel like this project was a valuable exploration into distributed programs, multithreading and UI control. We were also able to work effectively as a team using Git for version control, which is a valuable professional skill for the future.
