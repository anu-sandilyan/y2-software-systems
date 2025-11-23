
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "udp.h"

//initialise linked list for clients
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
    //add connecting client
    add_client(*sender, name);
    // convert client ip address to string, and port to int
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(sender->sin_addr), client_ip, INET_ADDRSTRLEN); 
    int client_port = ntohs(sender->sin_port);

    // print user details to server terminal
    printf("user '%s' added to active clients: ", name);
    printf("connected from IP: %s, port: %d\n", client_ip, client_port);

    // send response only to client who just connected
    char response[BUFFER_SIZE];
    snprintf(response, BUFFER_SIZE, "hi %s, you have successfully connected to the chat!", name);
    udp_socket_write(sd, sender, response, BUFFER_SIZE);
}

void disconnect_client(int sd, struct sockaddr_in *sender) {
    //start at top of client list
    ClientNode *current = head;
    ClientNode *prev = NULL;

    //find disconnecting client in list
    while (current != NULL) {
        // client port should match
        if (current->client_addr.sin_port == sender->sin_port) {
            if (prev == NULL) {
                head = current->next;
            } else {
                prev->next = current->next;
            }
            printf("user '%s' removed from active clients.\n", current->name);

            char response[] = "you have been disconnected. bye!";
            udp_socket_write(sd, sender, response, sizeof(response));
            // delete current node
            free(current);
            return;
        }   
        //move pointer to next node
        prev = current;
        current = current->next;
    }
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
        //command logic goes here
        if (strcmp(command, "conn") == 0) 
        {
            connect_client(sd, client_address, message);
        } else if (strcmp(command, "disconn") == 0)
        {
            disconnect_client(sd, client_address);
        } else
        {
            printf("unknown command: %s \n", command);
            char error_msg[BUFFER_SIZE];
            snprintf(error_msg, BUFFER_SIZE, "Error: Unknown command '%s'", command);
            udp_socket_write(sd, client_address, error_msg, BUFFER_SIZE);
        } //implement command logic here
    } else 
    { 
        printf("invalid message format: use 'command$ message\n");
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
    printf("server is listening on port: %d\n", SERVER_PORT);

    // Server main loop
    while (1) 
    {
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
        }
    }

    return 0;
}
