#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <pthread.h>
#include "udp.h"

pthread_rwlock_t lock = PTHREAD_RWLOCK_INITIALIZER;
//initialise linked list for clients

//linked list for mute status for all clients
typedef struct MutedClients {
    char name[32];
    struct MutedClients *next;
} MutedClients;
typedef struct ClientNode {
    struct sockaddr_in client_addr;
    // attribute fields
    char name[32]; 
    char client_ip[INET_ADDRSTRLEN]; 
    int client_port;
    struct ClientNode *next;
    MutedClients *muted_client;
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
    pthread_rwlock_wrlock(&lock); //CRITICAL SECTION - writing to global list (lock)
    new_node->next = head;
    head = new_node;
    pthread_rwlock_unlock(&lock); //END CRITICAL SECTION - unlock
}

/* const char* find_client(struct sockaddr_in *sender) {
    //convert port + ip to human readable format, to compare w client attributes
    int sender_port = ntohs(sender->sin_port);
    char sender_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(sender->sin_addr), sender_ip, INET_ADDRSTRLEN);
    // find client name
    pthread_rwlock_rdlock(&lock); //CRITICAL SECTION - reading global list (lock)
    for (ClientNode *cur = head; cur != NULL; cur = cur->next) {
        if (cur->client_port == sender_port && strcmp(cur->client_ip, sender_ip) == 0) {
            pthread_rwlock_unlock(&lock); //CRITICAL SECTION - reading global list (lock)
            return cur->name;
        }
    }
    pthread_rwlock_unlock(&lock); //END CRITICAL SECTION - unlock
    return "unknown error: client not found";
} */

ClientNode* find_client_node(struct sockaddr_in *sender) {
    int sender_port = ntohs(sender->sin_port);
    char sender_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(sender->sin_addr), sender_ip, INET_ADDRSTRLEN);

    for (ClientNode *current = head; current != NULL; current = current->next) {
        if (current->client_port == sender_port && strcmp(current->client_ip, sender_ip) == 0) {
            return current; // return node
        }
    }
    return NULL;
}

void connect_client(int sd, struct sockaddr_in *sender, char *name) {
    //add connecting client
    add_client(*sender, name);
    // print user details to server terminal
    printf("user '%s' added to active clients: ", name);
    printf("connected from IP: %s, port: %d\n", head->client_ip, head->client_port);

    // send response only to client who just connected
    char connect_msg[BUFFER_SIZE];
    snprintf(connect_msg, BUFFER_SIZE, "hi %s, you have successfully connected to the chat!", name);

    char join_msg[BUFFER_SIZE];
    snprintf(join_msg, BUFFER_SIZE, "%s has joined the chat.", name);
    // notify all clients about the new user

    pthread_rwlock_rdlock(&lock); //CRITICAL SECTION - reading global list (lock)
    ClientNode *current = head;
    while (current != NULL) {
        if (current->client_port == ntohs(sender->sin_port)) {
            // Send specific welcome message to the new user
             udp_socket_write(sd, &current->client_addr, connect_msg, BUFFER_SIZE);
        } else {
            // Send "X has joined" to everyone else
             udp_socket_write(sd, &current->client_addr, join_msg, BUFFER_SIZE);
        }
        current = current->next;
    }
    pthread_rwlock_unlock(&lock); //END CRITICAL SECTION - unlock
}

void disconnect_client(int sd, struct sockaddr_in *sender) {
    int sender_port = ntohs(sender->sin_port);
    //start at top of client list
    pthread_rwlock_wrlock(&lock); // CRITICAL SECTION - writing to global list (lock)
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
            MutedClients *m_cur = current->muted_client;
            while (m_cur != NULL) {
                MutedClients *m_next = m_cur->next;
                free(m_cur);
                m_cur = m_next;
            }
            free(current);
            pthread_rwlock_unlock(&lock); //END CRITICAL SECTION - unlock
            return;
        }   
        //move pointer to next node
        prev = current;
        current = current->next;

    }
    pthread_rwlock_unlock(&lock); //END CRITICAL SECTION - unlock
}

void kick_client(int sd, struct sockaddr_in *sender, char *target_name) {
    int sender_port = ntohs(sender->sin_port);
    if (sender_port != 6666) {
        printf("unauthorized permissions to kick user: attempt from port %d\n", sender_port);
        char error_msg[] = "error: only the server admin (port 6666) can kick users.";
        udp_socket_write(sd, sender, error_msg, sizeof(error_msg));
        return;
    }
    pthread_rwlock_wrlock(&lock); //CRITICAL SECTION - writing to global list (lock)
    // check if admin is sending the kick request
    ClientNode *admin_node = find_client_node(sender);
    printf("admin '%s' (Port 6666) is kicking user '%s'\n", admin_node->name, target_name);

    // if admin, find target user to kick
    ClientNode *current = head;
    ClientNode *prev = NULL;
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
            snprintf(global_msg, BUFFER_SIZE, "%s has been removed from the chat.", target_name);

            ClientNode *client = head;
            while (client != NULL) {
                if (client != current && ntohs(client->client_addr.sin_port) != 6666) {
                    udp_socket_write(sd, &client->client_addr, global_msg, BUFFER_SIZE);
                }
                client = client->next;
            }
            
            // Unlink the node from the list
            if (prev == NULL) {
                head = current->next; //removing head
            } else {
                prev->next = current->next; //removing from middle/end
            }

            printf("admin kicked user '%s'.\n", current->name);
            MutedClients *m_cur = current->muted_client;
            while (m_cur != NULL) {
                MutedClients *m_next = m_cur->next;
                free(m_cur);
                m_cur = m_next;
            }
            free(current);
            pthread_rwlock_unlock(&lock); //END CRITICAL SECTION - unlock
            return;
        }
        current = current->next;
    }

    // if target not found
    char not_found[BUFFER_SIZE];
    snprintf(not_found, BUFFER_SIZE, "error: user '%s' not found.", target_name);
    udp_socket_write(sd, sender, not_found, BUFFER_SIZE);
    pthread_rwlock_unlock(&lock); //END CRITICAL SECTION - unlock

}

void rename_client(int sd, struct sockaddr_in *sender, char *new_name) {
    int sender_port = ntohs(sender->sin_port);
    char sender_ip[INET_ADDRSTRLEN];
    pthread_rwlock_wrlock(&lock); //CRITICAL SECTION - writing to global list (lock)
    inet_ntop(AF_INET, &(sender->sin_addr), sender_ip, INET_ADDRSTRLEN);

    ClientNode *current = head;

    // find client
    while (current != NULL) {
        // match by Port AND IP
        if (current->client_port == sender_port && strcmp(current->client_ip, sender_ip) == 0) { 
            printf("user '%s' has changed name to '%s'\n", current->name, new_name);

            strncpy(current->name, new_name, 31);
            current->name[31] = '\0'; // ensure it is a valid string
            char response[BUFFER_SIZE];
            snprintf(response, BUFFER_SIZE, "you are now known as %s.", current->name);
            pthread_rwlock_unlock(&lock); //END CRITICAL SECTION - unlock
            udp_socket_write(sd, sender, response, BUFFER_SIZE);
            return;
        }
        current = current->next;
    }

    // if user not found
    pthread_rwlock_unlock(&lock); //END CRITICAL SECTION - unlock
    char error_msg[] = "error: user name not found.";
    udp_socket_write(sd, sender, error_msg, sizeof(error_msg));
}

static bool reciever_is_muted(const ClientNode *client, const char *reciever) { // CHECK: Has the reciever(2) muted the client(1)
    //if (!from_client || !target_name) return false;
    
    for (const MutedClients *m = client->muted_client; m != NULL; m = m->next) {
        if (strcmp(m->name, reciever) == 0) return true;
    }
    return false;
}

void mute_client(int sd, struct sockaddr_in *sender, char *target_name) {

    size_t n = strlen(target_name);
    while (n > 0 && (target_name[n-1] == '\r' || target_name[n-1] == '\n' || target_name[n-1] == ' ')) {
        target_name[--n] = '\0';
    }

    pthread_rwlock_wrlock(&lock); //CRITICAL SECTION - writing to global list (lock)

    bool target_found = false;
    for (ClientNode *curr = head; curr != NULL; curr = curr->next) {
        if (strcmp(curr->name, target_name) == 0) {
            target_found = true;
            break;
        }
    }

    if (!target_found) {
        char client_msg[BUFFER_SIZE];
        snprintf(client_msg, BUFFER_SIZE, "Action Failed: There is no client with name '%s'", target_name);
        udp_socket_write(sd, sender, client_msg, BUFFER_SIZE);
        pthread_rwlock_unlock(&lock); //END CRITICAL SECTION - unlock
        return;
    }

    ClientNode* from_client = find_client_node (sender);

    if (reciever_is_muted(from_client, target_name)) {
        char client_msg[BUFFER_SIZE];
        snprintf(client_msg, BUFFER_SIZE, "Action Failed: You have already muted %s", target_name);
        udp_socket_write(sd, sender, client_msg, BUFFER_SIZE);
        pthread_rwlock_unlock(&lock); //END CRITICAL SECTION - unlock
        return;
    }

    MutedClients *new_node = malloc(sizeof(MutedClients));
    strncpy(new_node->name, target_name, 31);
    new_node->name[31] = '\0';

    new_node->next = from_client->muted_client;
    from_client->muted_client = new_node;

    char client_msg[BUFFER_SIZE];
    snprintf(client_msg, BUFFER_SIZE, "%s has been muted.", target_name);
    udp_socket_write(sd, sender, client_msg, BUFFER_SIZE);
    pthread_rwlock_unlock(&lock); //END CRITICAL SECTION - unlock
            
        
}

void unmute_client(int sd, struct sockaddr_in *sender, char *target_name) {

    size_t n = strlen(target_name);
    while (n > 0 && (target_name[n-1] == '\r' || target_name[n-1] == '\n' || target_name[n-1] == ' ')) {
        target_name[--n] = '\0';
    }

    pthread_rwlock_wrlock(&lock); //CRITICAL SECTION - writing to global list (lock)

    bool target_found = false;
    for (ClientNode *curr = head; curr != NULL; curr = curr->next) {
        if (strcmp(curr->name, target_name) == 0) {
            target_found = true;
            break;
        }
    }

    if (!target_found) {
        char client_msg[BUFFER_SIZE];
        snprintf(client_msg, BUFFER_SIZE, "Action Failed: There is no client with name '%s'", target_name);
        udp_socket_write(sd, sender, client_msg, BUFFER_SIZE);
        pthread_rwlock_unlock(&lock); //END CRITICAL SECTION - unlock
        return;
    }

    ClientNode* from_client = find_client_node(sender);

    if (reciever_is_muted(from_client, target_name)) {
        //start at top of client list
        MutedClients *prev = NULL;
        MutedClients *current = from_client->muted_client;

        while (current != NULL) {
        // client port should match
            if (strcmp(current->name, target_name) == 0) {
                if (prev == NULL) {
                    ((ClientNode*)from_client)->muted_client = current->next;  // Removing head: update the client's list head
                } else {
                    prev->next = current->next;
                }

                char client_msg[BUFFER_SIZE];
                snprintf(client_msg, BUFFER_SIZE, "%s has been unmuted", target_name);
                udp_socket_write(sd, sender, client_msg, BUFFER_SIZE);

                // delete current node
                free(current);
                pthread_rwlock_unlock(&lock); //END CRITICAL SECTION - unlock
                return;
            }   
            //move pointer to next node
            prev = current;
            current = current->next;
        }
        pthread_rwlock_unlock(&lock); //END CRITICAL SECTION - unlock

    }
    else {
        char client_msg[BUFFER_SIZE];
        snprintf(client_msg, BUFFER_SIZE, "Action Failed: %s is already unmuted", target_name);
        udp_socket_write(sd, sender, client_msg, BUFFER_SIZE);
        pthread_rwlock_unlock(&lock); //END CRITICAL SECTION - unlock

    }
}

void say_client(int sd, struct sockaddr_in *sender, char *message) {
    pthread_rwlock_rdlock(&lock); //CRITICAL SECTION - reading global list (lock)
    
    const ClientNode* from_client = find_client_node (sender);
    char global_msg[BUFFER_SIZE];
    int n = snprintf(global_msg, sizeof(global_msg), "%s: %s", from_client->name, message);
    if (n < 0) return;
    size_t resp_len = (size_t)n;

    for (ClientNode *cur = head; cur != NULL; cur = cur->next) {
        if (!reciever_is_muted(cur, from_client->name)) {
        udp_socket_write(sd, &cur->client_addr, global_msg, (int)resp_len);
        }
    }   
    pthread_rwlock_unlock(&lock); //END CRITICAL SECTION - unlock
}

void sayto_client(int sd, struct sockaddr_in *sender, char *nameandmessage ) {

size_t idx = strcspn(nameandmessage, " ");
    
    // Parsing logic stays outside the lock for performance
    if (nameandmessage[idx] != ' ') {
        char err[] = "error: format command as 'sayto$ Name Message'";
        udp_socket_write(sd, sender, err, sizeof(err));
        return;
    }

    nameandmessage[idx] = '\0';
    const char *target_name = nameandmessage;
    const char *message = nameandmessage + idx + 1;

    // 1. LOCK List for Reading (Shared Access)
    pthread_rwlock_rdlock(&lock);

    // 2. Find Sender Node Safely inside the lock
    const ClientNode* from_client = find_client_node(sender);
    
    // Safety Check: Sender may have disconnected
    if (from_client == NULL) {
        pthread_rwlock_unlock(&lock);
        return;
    }

    char global_msg[BUFFER_SIZE];
    int n = snprintf(global_msg, sizeof(global_msg), "%s (private): %s", from_client->name, message);
    if (n < 0) {
        pthread_rwlock_unlock(&lock);
        return;
    }
    size_t resp_len = (size_t)n;

    // 3. Broadcast Loop
    int target_found = 0;
    ClientNode *client = head;
    while (client != NULL) {
        // Condition: Recipient is the target name OR is the sender themselves
        if (strcmp(client->name, target_name) == 0 || client == from_client) {
            
            // SECURITY CHECK: Has the recipient muted the sender?
            if (!reciever_is_muted(client, from_client->name)) {
                udp_socket_write(sd, &client->client_addr, global_msg, (int)resp_len);
            }
            
            if (strcmp(client->name, target_name) == 0) target_found = 1;
        }
        client = client->next;
    }

    // 4. UNLOCK
    pthread_rwlock_unlock(&lock);

    // Optional: Notify sender if user was not found
    if (!target_found) {
        char err_msg[BUFFER_SIZE];
        snprintf(err_msg, BUFFER_SIZE, "error: user '%s' not found.", target_name);
        udp_socket_write(sd, sender, err_msg, BUFFER_SIZE);
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
        else if (strcmp(command, "rename") == 0) 
        {
            rename_client(sd, client_address, message);
        }
        else if (strcmp(command, "say") == 0)
        {
            say_client(sd, client_address, message);
        } 
        else if (strcmp(command, "sayto") == 0)
        {
            sayto_client(sd, client_address, message);
        }  
        else if (strcmp(command, "mute") == 0)
        {
            mute_client(sd, client_address, message);
        }  
        else if (strcmp(command, "unmute") == 0)
        {
            unmute_client(sd, client_address, message);
        }  
        else 
        {
            char error_msg[BUFFER_SIZE];
            snprintf(error_msg, BUFFER_SIZE, "error: unknown command '%s'", command);
            udp_socket_write(sd, client_address, error_msg, BUFFER_SIZE);
        } //implement command logic here
    } else 
    { 
        char error_msg[BUFFER_SIZE];
        snprintf(error_msg, BUFFER_SIZE, "invalid message format: use 'command$ message");
        udp_socket_write(sd, client_address, error_msg, BUFFER_SIZE);
    }
}

//data structure to store arguments to pass to thread (client address, socket descriptor, request message)
typedef struct {
    int sd;
    struct sockaddr_in client_addr;
    char request[BUFFER_SIZE];
} ThreadArgs;

//function for multithreading
void *handle_request_thread(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    parse_request(args->request, args->sd, &args->client_addr);
    free(args);
    return NULL;
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
            //create new thread
            pthread_t tid;
            ThreadArgs *args = malloc(sizeof(ThreadArgs));
            if (args == NULL) {
                perror("error: failed to allocate memory for thread arguments");
                continue;
            }
            args->sd = sd;
            args->client_addr = client_address;
            strncpy(args->request, client_request, BUFFER_SIZE);

            if (pthread_create(&tid, NULL, handle_request_thread, args) != 0) {
                perror("thread create failed");
                free(args);
            } else {
                pthread_detach(tid); // clean up thread resources
            }


            // This function writes back to the incoming client,
            // whose address is now available in client_address, 
            // through the socket at sd.
            // (See details of the function in udp.h)
            //rc = udp_socket_write(sd, &client_address, server_response, BUFFER_SIZE);
        }
    }

    return 0;
}
