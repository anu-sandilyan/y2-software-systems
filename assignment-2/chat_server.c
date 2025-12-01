
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "udp.h"

//initialise linked list for clients
typedef struct ClientNode {
    struct sockaddr_in client_addr;
    // attribute fields
    char name[32]; 
    char client_ip[INET_ADDRSTRLEN]; 
    int client_port;
    struct ClientNode *next;
} ClientNode;
ClientNode *head = NULL;

void add_client(struct sockaddr_in addr, char *name) {
    ClientNode *new_node = malloc(sizeof(ClientNode));
    new_node->client_addr = addr;
    new_node->client_port = ntohs(addr.sin_port);

    // 2. Store the IP as a string
    inet_ntop(AF_INET, &(addr.sin_addr), new_node->client_ip, INET_ADDRSTRLEN);
    //copy client name into the list
    strncpy(new_node->name, name, 31);
    new_node->name[31] = '\0';

    new_node->next = head;
    head = new_node;
}

const char* find_sender(struct sockaddr_in *sender) {
    // Find sender name
    for (ClientNode *cur = head; cur != NULL; cur = cur->next) {
        if (cur->client_addr.sin_addr.s_addr == sender->sin_addr.s_addr &&
            cur->client_addr.sin_port == sender->sin_port) {
            return cur->name;
        }
    }
    return "unknown";
}

void connect_client(int sd, struct sockaddr_in *sender, char *name) {
    //add connecting client
    add_client(*sender, name);
    // print user details to server terminal
    printf("user '%s' added to active clients: ", name);
    printf("connected from IP: %s, port: %d\n", head->client_ip, head->client_port);

    // send response only to client who just connected
    char response[BUFFER_SIZE];
    snprintf(response, BUFFER_SIZE, "hi %s, you have successfully connected to the chat!", name);
    udp_socket_write(sd, sender, response, BUFFER_SIZE);
}

void disconnect_client(int sd, struct sockaddr_in *sender) {
    int sender_port = ntohs(sender->sin_port);
    //start at top of client list
    ClientNode *current = head;
    ClientNode *prev = NULL;

    //find disconnecting client in list
    while (current != NULL) {
        // client port should match
        if (current->client_port == sender_port) {
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

void kick_client(int sd, struct sockaddr_in *sender, char *target_name) {
    // check if admin is sending the kick request
    int sender_port = ntohs(sender->sin_port);
    if (sender_port != 6666) {
        printf("unauthorized permissions to kick user: attempt from port %d\n", sender_port);
        char error_msg[] = "error: only the server admin (port 6666) can kick users.";
        udp_socket_write(sd, sender, error_msg, sizeof(error_msg));
        return;
    }

    // if admin, find target user to kick
    ClientNode *current = head;
    while (current != NULL) {

        if (strcmp(current->name, target_name) == 0) {
            //show to admin
            char admin_msg[BUFFER_SIZE];
            snprintf(admin_msg, BUFFER_SIZE, "success: %s has been removed.", target_name);
            udp_socket_write(sd, sender, admin_msg, BUFFER_SIZE);

            //notify target that they have been kicked
            char kick_msg[] = "you have been removed from the chat.";
            udp_socket_write(sd, &current->client_addr, kick_msg, sizeof(kick_msg));
            
            // notify everyone else (except admin and target)
            char global_msg[BUFFER_SIZE];
            snprintf(global_msg, BUFFER_SIZE, "%s has been removed from the chat", target_name);

            ClientNode *client = head;
            while (client != NULL) {
                if (client != current && ntohs(client->client_addr.sin_port) != 6666) {
                    udp_socket_write(sd, &client->client_addr, global_msg, BUFFER_SIZE);
                }
                client = client->next;
            }

            disconnect_client(sd, &current->client_addr);      
            return;
        }
        current = current->next;
    }

    // if target not found
    char not_found[BUFFER_SIZE];
    snprintf(not_found, BUFFER_SIZE, "error: user '%s' not found.", target_name);
    udp_socket_write(sd, sender, not_found, BUFFER_SIZE);
}

void say_client(int sd, struct sockaddr_in *sender, char *message) {
    
    const char *from = find_sender(sender);

    char global_msg[BUFFER_SIZE];
    int n = snprintf(global_msg, sizeof(global_msg), "%s: %s", from, message);
    if (n < 0) return;
    size_t resp_len = (size_t)n;

    for (ClientNode *cur = head; cur != NULL; cur = cur->next) {
        udp_socket_write(sd, &cur->client_addr, global_msg, (int)resp_len);
    }   
}

void sayto_client(int sd, struct sockaddr_in *sender, char *sendtoandmessage ) {

    size_t idx = strcspn(sendtoandmessage, " ");
    if (sendtoandmessage[idx] == ' ') {
        sendtoandmessage[idx] = '\0';
        const char *sendto = sendtoandmessage;
        const char *message = sendtoandmessage + idx + 1;
        //while (*rest == ' ') rest++; // skip extra spaces

    const char *from = find_sender(sender);

    char global_msg[BUFFER_SIZE];
    int n = snprintf(global_msg, sizeof(global_msg), "%s: %s", from, message);
    if (n < 0) return;
    size_t resp_len = (size_t)n;


    ClientNode *client = head;
    while (client != NULL) {
        if ( strcmp(client->name, sendto) == 0 || strcmp(client->name, from) == 0 ) { //|| client == from) {
            udp_socket_write(sd, &client->client_addr, global_msg, (int)resp_len);
        }
        client = client->next;
    }



    //for (ClientNode *cur = head; cur != NULL; cur = cur->next) {
      //  udp_socket_write(sd, &cur->client_addr, global_msg, (int)resp_len);
    //}
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
        } 
        else if (strcmp(command, "disconn") == 0)
        {
            disconnect_client(sd, client_address);
        } 
        else if (strcmp(command, "kick") == 0)
        {
            kick_client(sd, client_address, message);
        } 
        else if (strcmp(command, "say") == 0)
        {
            say_client(sd, client_address, message);
        } 
        else if (strcmp(command, "sayto") == 0)
        {
            sayto_client(sd, client_address, message);
        }    
        else
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
