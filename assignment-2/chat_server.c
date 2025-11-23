
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "udp.h"


//initialise linked list of clients
typedef struct ClientNode {
    struct sockaddr_in client_addr;
    char name[32]; // field for names
    struct ClientNode *next;
} ClientNode;
ClientNode *head = NULL;

void add_client(struct sockaddr_in addr, char *name) {
    ClientNode *new_node = malloc(sizeof(ClientNode));
    new_node->client_addr = addr;
    //copy client name into the list
    strncpy(new_node->name, name, 31);
    new_node->name[31] = '\0';

    new_node->next = head;
    head = new_node;
}

void connect_client(int sd, struct sockaddr_in *sender, char *name) {
    //add client sending message
    add_client(*sender, name);
    printf("user '%s' added to active clients.\n", name);

    // send response to client
    char response[BUFFER_SIZE];
    snprintf(response, BUFFER_SIZE, "hi %s, you have successfully connected to the chat!", name);
    
    // send only to the person who just connected
    udp_socket_write(sd, sender, response, BUFFER_SIZE);
}

void parse_request(char *client_request, int sd, struct sockaddr_in *client_address){
        //split string into command + message, with $ delimiter
    char *delimiter = strchr(client_request, '$');
    if (delimiter != NULL) {
        // replace '$' with null terminator to split string
        *delimiter = '\0'; 

        // Nfirst part is the command
        char *command = client_request;

        // The character immediately after the old '$' is the start of the content (" Hello World")
        // add +1 to skip null terminator index
        char *message = delimiter + 1;
        //removing leading space from message if needed
        if (*message == ' '){
            message++; 
        }

        printf("Command: %s, Message: %s\n", command, message);
        //command logic goes here
        if (strcmp(command, "conn") == 0) {
            connect_client(sd, client_address, message);
        } else {
            printf("unknown command: %s \n", command);
            char error_msg[BUFFER_SIZE];
            snprintf(error_msg, BUFFER_SIZE, "Error: Unknown command '%s'", command);
            udp_socket_write(sd, client_address, error_msg, BUFFER_SIZE);
        } //implement command logic here
    } else { 
        printf("invalid message format: use 'command$ message");
    }
}

int main(int argc, char *argv[])
{

    // This function opens a UDP socket,
    // binding it to all IP interfaces of this machine,
    // and port number SERVER_PORT
    // (See details of the function in udp.h)
    int sd = udp_socket_open(SERVER_PORT);

    assert(sd > -1);

    // Server main loop
    while (1) 
    {
        printf("server is listening on port: %d\n", SERVER_PORT);
        // Storage for request and response messages
        char client_request[BUFFER_SIZE], server_response[BUFFER_SIZE];
        // Variable to store incoming client's IP address and port
        struct sockaddr_in client_address;
    
        // This function reads incoming client request from
        // the socket at sd.
        // (See details of the function in udp.h)
        int rc = udp_socket_read(sd, &client_address, client_request, BUFFER_SIZE);

        // Successfully received an incoming request
        if (rc > 0)
        {
            if (rc < BUFFER_SIZE) {
                client_request[rc] = '\0';
            } else {
                client_request[BUFFER_SIZE - 1] = '\0';
            }
            parse_request(client_request, sd, &client_address);

            // This function writes back to the incoming client,
            // whose address is now available in client_address, 
            // through the socket at sd.
            // (See details of the function in udp.h)
            //rc = udp_socket_write(sd, &client_address, server_response, BUFFER_SIZE);

            // Demo code (remove later)
            //printf("Request served...\n");
        }
    }

    return 0;
}
