

#include <stdio.h>
#include "udp.h"
#include <stdlib.h>    
#include <pthread.h>    // for threads
#include <string.h>     // for strcspn, strcmp
#include <ncurses.h>   // for terminal window UI control

WINDOW *chat_window;
WINDOW *input_window;
pthread_mutex_t screen_mutex = PTHREAD_MUTEX_INITIALIZER;

//port = 0 -> assigns any free port
#define CLIENT_PORT 0

//moved code in main to be handled by read and write threads
void *sender_thread(void *arg) {
    int sd = *((int *)arg); // get socket descriptor
    // Variable to store the server's IP address and port
    // (i.e. the server we are trying to contact).
    // Generally, it is possible for the responder to be
    // different from the server requested.
    // Although, in our case the responder will
    // always be the same as the server.
    struct sockaddr_in server_addr;
    
    // initialise server address (Localhost, Port 12000)
    // We are currently running the server on localhost (127.0.0.1).
    // (See details of the function in udp.h)
    set_socket_addr(&server_addr, "127.0.0.1", SERVER_PORT);

    // Storage for request and response messages
    char client_request[BUFFER_SIZE];
    while (1) {
        // wgetnstr reads user message
        wgetnstr(input_window, client_request, BUFFER_SIZE - 1);
        if (strlen(client_request) == 0) continue;
        //remove /n char at the end
        client_request[strcspn(client_request, "\n")] = 0;
        // This function writes to the server (sends request)
        // through the socket at sd.
        // (See details of the function in udp.h)
        udp_socket_write(sd, &server_addr, client_request, BUFFER_SIZE);
        if (strcmp(client_request, "exit$") == 0) {
            break; // exit program fully
        }
        pthread_mutex_lock(&screen_mutex);
        wclear(input_window);        // clear window
        box(input_window, 0, 0);
        mvwprintw(input_window, 1, 1, "> "); // reprint prompt char
        wrefresh(input_window);      
        pthread_mutex_unlock(&screen_mutex);
    }
    return NULL;
}

void *listener_thread(void *arg) {
    int sd = *((int *)arg);
    struct sockaddr_in sender_addr;
    char server_response[BUFFER_SIZE];

    while (1) {
        // This function reads the response from the server
        // through the socket at sd.
        // In our case, responder_addr will simply be
        // the same as server_addr.
        // (See details of the function in udp.h)
        int rc = udp_socket_read(sd, &sender_addr, server_response, BUFFER_SIZE);
        if (rc <= 0) { //happens on socket shutdown
            break; 
        } else {
            if (rc < BUFFER_SIZE) {
                server_response[rc] = '\0';
            }
            pthread_mutex_lock(&screen_mutex);
            wprintw(chat_window, " %s\n", server_response);
            box(chat_window, 0, 0);
            wrefresh(chat_window); //refresh displays printw to client
            wrefresh(input_window); //allows user to continue typing
            pthread_mutex_unlock(&screen_mutex);
            //removed printf, use ncurses to print to chat window instead
        }
    }
    return NULL;
}

void setup_screen() {
    initscr();          
    cbreak();             // disable line buffering (pass characters immediately)
    echo();               // show typed characters
    keypad(stdscr, TRUE); // enable function keys

    int height, width;
    getmaxyx(stdscr, height, width);

    int input_height = 3;
    int chat_height = height - input_height;

    // create 2 windows
    chat_window = newwin(chat_height, width, 0, 0);
    input_window = newwin(input_height, width, chat_height, 0);
    scrollok(chat_window, TRUE); 

    // tell Ncurses to only scroll lines 1 to (Height-2), so messages aren't cut off
    wsetscrreg(chat_window, 1, chat_height - 2);
    // make first message print at (1, 1) instead of (0, 0)
    wmove(chat_window, 1, 0);
    // draw borders
    box(chat_window, 0, 0);
    box(input_window, 0, 0);
    
    // '>' prompt
    mvwprintw(input_window, 1, 1, "> ");

    wrefresh(chat_window);
    wrefresh(input_window);
}

// client code
int main(int argc, char *argv[])
{
    int client_port = CLIENT_PORT;
    if (argc > 1) {
    client_port = atoi(argv[1]);
    }

    // This function opens a UDP socket,
    // binding it to all IP interfaces of this machine,
    // and dynamic port number user_port.
    // (See details of the function in udp.h)
    int sd = udp_socket_open(client_port);
    setup_screen();

    // initialise sender + listener threads w ids
    pthread_t send_tid, listen_tid;

    // create sender and listener threads for sd
    pthread_create(&send_tid, NULL, sender_thread, &sd);
    pthread_create(&listen_tid, NULL, listener_thread, &sd);

    // wait for threads to exit (when program terminates)
    pthread_join(send_tid, NULL);
    shutdown(sd, SHUT_RDWR);
    pthread_join(listen_tid, NULL);
    endwin(); //end ncurses mode
    close(sd);
    system("clear");
    printf("you have exited the chat program. \n");
    return 0;
}
