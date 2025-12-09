
#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <pthread.h>
#include "udp.h"

pthread_rwlock_t lock = PTHREAD_RWLOCK_INITIALIZER;

//linked list for mute status for all clients
typedef struct MutedClients {
    char name[32];
    struct MutedClients *next;
} MutedClients;
//MutedClients *mhead = NULL;

//initialise linked list for clients
typedef struct ClientNode {
    struct sockaddr_in client_addr;
    // attribute fields
    char name[32]; 
    char client_ip[INET_ADDRSTRLEN]; 
    int client_port;
    struct ClientNode *next;
    MutedClients *Muted_Client;
    
} ClientNode;
ClientNode *head = NULL;

static bool reciever_is_muted(const ClientNode *client, const char *reciever) { // CHECK: Has the reciever(2) muted the client(1)
    //if (!from_client || !target_name) return false;
    
    for (const MutedClients *m = client->Muted_Client; m != NULL; m = m->next) {
        if (strcmp(m->name, reciever) == 0) return true;
    }
    return false;
}

const ClientNode* find_clientnode(struct sockaddr_in *sender) {
    // Find sender name
    for (ClientNode *cur = head; cur != NULL; cur = cur->next) {
        if (cur->client_addr.sin_addr.s_addr == sender->sin_addr.s_addr &&
            cur->client_addr.sin_port == sender->sin_port) {
            return cur;
        }
    }
    return NULL;
}

void mute_client(int sd, struct sockaddr_in *sender, char *target_name) {

    const ClientNode* from_client = find_clientnode (sender);

    size_t n = strlen(target_name);
    while (n > 0 && (target_name[n-1] == '\r' || target_name[n-1] == '\n' || target_name[n-1] == ' ')) {
        target_name[--n] = '\0';
    }

    if (!from_client) {
        char client_msg[BUFFER_SIZE];
        snprintf(client_msg, BUFFER_SIZE, "Action Failed: There is no client with name '%s'", target_name);
        udp_socket_write(sd, sender, client_msg, BUFFER_SIZE);
        return;
    }


    if (reciever_is_muted(from_client, target_name)) {
        char client_msg[BUFFER_SIZE];
        snprintf(client_msg, BUFFER_SIZE, "Action Failed: You have already muted %s", target_name);
        udp_socket_write(sd, sender, client_msg, BUFFER_SIZE);
        return;
    }

    else {
        for (ClientNode *cur = head; cur != NULL; cur = cur->next) {
            if ( strcmp(cur->name, from_client->name) == 0 ) {
                
                MutedClients *new_node = (MutedClients *)malloc(sizeof(MutedClients));
                strncpy(new_node->name, target_name, 31);
                new_node->name[31] = '\0';

                new_node->next = cur->Muted_Client;   // push onto this sender client's list
                cur->Muted_Client = new_node;

                char client_msg[BUFFER_SIZE];
                snprintf(client_msg, BUFFER_SIZE, "%s has been muted", target_name);
                udp_socket_write(sd, sender, client_msg, BUFFER_SIZE);
                
            }
        }
    }
}

void unmute_client(int sd, struct sockaddr_in *sender, char *target_name) {

    const ClientNode* from_client = find_clientnode(sender);

    if (!from_client) {
        char client_msg[] = "Action Failed: acting client not found";
        udp_socket_write(sd, sender, client_msg, (int)strlen(client_msg));
        return;
    }

    size_t tn = strlen(target_name);
    while (tn > 0 && (target_name[tn-1] == '\r' || target_name[tn-1] == '\n' || target_name[tn-1] == ' ')) {
        target_name[--tn] = '\0';
    }

    if (reciever_is_muted(from_client, target_name)) {
        //start at top of client list
        MutedClients *prev = NULL;
        MutedClients *current = from_client->Muted_Client;

        while (current != NULL) {
        // client port should match
            if (strcmp(current->name, target_name) == 0) {
                if (prev == NULL) {
                    ((ClientNode*)from_client)->Muted_Client = current->next;  // Removing head: update the client's list head
                } else {
                    prev->next = current->next;
                }

                char client_msg[BUFFER_SIZE];
                snprintf(client_msg, BUFFER_SIZE, "%s has been unmuted", target_name);
                udp_socket_write(sd, sender, client_msg, BUFFER_SIZE);

                // delete current node
                free(current);
                return;
            }   
            //move pointer to next node
            prev = current;
            current = current->next;
        }
    }

    else {
        char client_msg[BUFFER_SIZE];
        snprintf(client_msg, BUFFER_SIZE, "Action Failed: %s is already unmuted", target_name);
        udp_socket_write(sd, sender, client_msg, BUFFER_SIZE);
    }
}

void add_client(struct sockaddr_in addr, char *name) {
    ClientNode *new_node = malloc(sizeof(ClientNode));
    new_node->client_addr = addr;
    new_node->client_port = ntohs(addr.sin_port);

    // 2. Store the IP as a string
    inet_ntop(AF_INET, &(addr.sin_addr), new_node->client_ip, INET_ADDRSTRLEN);

    //copy client name into the list
    strncpy(new_node->name, name, 31);
    new_node->name[31] = '\0';

    // Push client onto client list
    new_node->next = head;
    new_node->Muted_Client = NULL; // initialise empty mute list
    head = new_node;

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

    const ClientNode* client = find_clientnode (sender);
    int sender_port = ntohs(sender->sin_port);
    //start at top of client list
    ClientNode *current = head;
    ClientNode *prev = NULL;

    for (ClientNode *cur = head; cur != NULL; cur = cur->next) {
        if (reciever_is_muted(client, cur->name)) {
            unmute_client(sd, sender,cur->name);
        }
    }


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
    
    const ClientNode* from_client = find_clientnode (sender);

    char global_msg[BUFFER_SIZE];
    int n = snprintf(global_msg, sizeof(global_msg), "%s: %s", from_client->name, message);
    if (n < 0) return;
    size_t resp_len = (size_t)n;

    for (ClientNode *cur = head; cur != NULL; cur = cur->next) {
        if (!reciever_is_muted(cur, from_client->name)) {
        udp_socket_write(sd, &cur->client_addr, global_msg, (int)resp_len);
        }
    }   

}

void sayto_client(int sd, struct sockaddr_in *sender, char *sendtoandmessage ) {

    size_t idx = strcspn(sendtoandmessage, " ");
    if (sendtoandmessage[idx] == ' ') {
        sendtoandmessage[idx] = '\0';
        const char *sendto = sendtoandmessage;
        const char *message = sendtoandmessage + idx + 1;
        //while (*rest == ' ') rest++; // skip extra spaces

        const ClientNode* from_client = find_clientnode (sender);

        char global_msg[BUFFER_SIZE];
        int n = snprintf(global_msg, sizeof(global_msg), "%s: %s", from_client->name, message);
        if (n < 0) return;
        size_t resp_len = (size_t)n;


        ClientNode *client = head;
        while (client != NULL) {
            if ( strcmp(client->name, sendto) == 0 || strcmp(client->name, from_client->name) == 0 ) {
                if (!reciever_is_muted(client,from_client->name)) { //|| client == from) {
                    udp_socket_write(sd, &client->client_addr, global_msg, (int)resp_len);
                }
            }
            client = client->next;
        }

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
        else if (strcmp(command, "kick") == 0){
            kick_client(sd, client_address, message);
        } 
        else if (strcmp(command, "say") == 0){
            say_client(sd, client_address, message);
        } 
        else if (strcmp(command, "sayto") == 0){
            sayto_client(sd, client_address, message);
        }          
        else if (strcmp(command, "mute") == 0){
            mute_client(sd, client_address, message);
        }   
        else if (strcmp(command, "unmute") == 0){
            unmute_client(sd, client_address, message);
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

int main(int argc, char *argv[]){

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
