

#include <stdio.h>
#include "udp.h"
#include <stdlib.h>    
#include <pthread.h>    // for threads
#include <string.h>     // for strcspn, strcmp

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
    // You can change this to a different IP address
    // when running the server on a different machine.
    // (See details of the function in udp.h)
    set_socket_addr(&server_addr, "127.0.0.1", SERVER_PORT);

    // Storage for request and response messages
    char client_request[BUFFER_SIZE];
    while (1) {
        // stdin reads user message
        fgets(client_request, BUFFER_SIZE, stdin);
        //remove /n char at the end
        client_request[strcspn(client_request, "\n")] = 0;
        // This function writes to the server (sends request)
        // through the socket at sd.
        // (See details of the function in udp.h)
        udp_socket_write(sd, &server_addr, client_request, BUFFER_SIZE);
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
        if (rc > 0) {
            // Safety: null terminate
            if (rc < BUFFER_SIZE) server_response[rc] = '\0';
            printf("%s\n", server_response);
        }
    }
    return NULL;
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

    // initialise sender + listener threads w ids
    pthread_t send_tid, listen_tid;

    // create sender and listener threads for sd
    pthread_create(&send_tid, NULL, sender_thread, &sd);
    pthread_create(&listen_tid, NULL, listener_thread, &sd);

    // wait for threads to exit (when program terminates)
    pthread_join(send_tid, NULL);
    pthread_join(listen_tid, NULL);
    return 0;
}
