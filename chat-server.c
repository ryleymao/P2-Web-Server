#include "http-server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define MAX_CHAT_LENGTH 255
#define MAX_USERNAME_LENGTH 15
#define MAX_REACTIONS 100
#define MAX_CHATS 100000

typedef struct Reaction {
    char username[MAX_USERNAME_LENGTH + 1];
    char reaction[MAX_CHAT_LENGTH + 1];
} Reaction;

typedef struct Chat {
    int id;
    char username[MAX_USERNAME_LENGTH + 1];
    char message[MAX_CHAT_LENGTH + 1];
    time_t timestamp;
    Reaction reactions[MAX_REACTIONS];
    int num_reactions;
} Chat;

Chat *chats = NULL; // Dynamically allocated array of chats
static int chat_count = 0; // Static variable to track the number of chats

// Function to decode URL-encoded strings (e.g., %20 -> space)
void url_decode(char *str) {
    char *pstr = str;
    char *pdecoded = str;
    while (*pstr) {
        if (*pstr == '%' && isxdigit(*(pstr + 1)) && isxdigit(*(pstr + 2))) {
            sscanf(pstr + 1, "%2hhx", pdecoded);
            pstr += 3;
        } else {
            *pdecoded = *pstr;
            pstr++;
        }
        pdecoded++;
    }
    *pdecoded = '\0';
}

void print_chat(Chat *chat) {
    struct tm *time_info = localtime(&chat->timestamp);
    char time_str[20];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", time_info);  // Correct timestamp format

    printf("[#%d %s]          %s: %s\n", chat->id, time_str, chat->username, chat->message);

    // Print reactions
    for (int i = 0; i < chat->num_reactions; i++) {
        printf("                              (%s)  %s\n", chat->reactions[i].username, chat->reactions[i].reaction);
    }
}

void send_response(int client_sock, const char *status, const char *content_type, const char *body) {
    char response[1024];
    snprintf(response, sizeof(response),
        "HTTP/1.1 %s\r\n"
        "Content-Type: %s\r\n"
        "Connection: close\r\n"
        "Content-Length: %ld\r\n"
        "\r\n"
        "%s",
        status, content_type, strlen(body), body);
    send(client_sock, response, strlen(response), 0);
}

void handle_get_chats(int client_sock) {
    char body[2048] = "";

    for (int i = 0; i < chat_count; i++) {
        char chat_buffer[1024];
        struct tm *time_info = localtime(&chats[i].timestamp);
        char time_str[20];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", time_info);  // Correct timestamp format

        snprintf(chat_buffer, sizeof(chat_buffer), "[#%d %s]          %s: %s\n", chats[i].id, time_str, chats[i].username, chats[i].message);
        strcat(body, chat_buffer);

        for (int j = 0; j < chats[i].num_reactions; j++) {
            snprintf(chat_buffer, sizeof(chat_buffer), "                              (%s)  %s\n", chats[i].reactions[j].username, chats[i].reactions[j].reaction);
            strcat(body, chat_buffer);
        }
    }

    if (strlen(body) == 0) {
        send_response(client_sock, "404 Not Found", "text/plain", "No chats available.");
    } else {
        send_response(client_sock, "200 OK", "text/plain", body);
    }
}

void handle_post_message(int client_sock, const char *user, const char *message) {
    if (strlen(user) > MAX_USERNAME_LENGTH || strlen(message) > MAX_CHAT_LENGTH) {
        send_response(client_sock, "400 Bad Request", "text/plain", "Username or message too long.");
        return;
    }

    if (chat_count >= MAX_CHATS) {
        send_response(client_sock, "500 Internal Server Error", "text/plain", "Too many chats.");
        return;
    }

    if (chat_count == 0) {
        chats = (Chat *)malloc(sizeof(Chat) * MAX_CHATS); // Dynamically allocate memory for chats
    }

    chats[chat_count].id = chat_count + 1;
    strncpy(chats[chat_count].username, user, MAX_USERNAME_LENGTH);
    strncpy(chats[chat_count].message, message, MAX_CHAT_LENGTH);
    chats[chat_count].timestamp = time(NULL);
    chats[chat_count].num_reactions = 0;
    chat_count++;

    handle_get_chats(client_sock);  // Send updated chat list
}

void handle_post_reaction(int client_sock, const char *user, const char *reaction, int chat_id) {
    if (strlen(user) > MAX_USERNAME_LENGTH || strlen(reaction) > MAX_CHAT_LENGTH) {
        send_response(client_sock, "400 Bad Request", "text/plain", "Username or reaction too long.");
        return;
    }

    if (chat_id < 1 || chat_id > chat_count) {
        send_response(client_sock, "400 Bad Request", "text/plain", "Chat ID does not exist.");
        return;
    }

    if (chats[chat_id - 1].num_reactions >= MAX_REACTIONS) {
        send_response(client_sock, "500 Internal Server Error", "text/plain", "Too many reactions.");
        return;
    }

    Reaction new_reaction = {.username = "", .reaction = ""};
    strncpy(new_reaction.username, user, MAX_USERNAME_LENGTH);
    strncpy(new_reaction.reaction, reaction, MAX_CHAT_LENGTH);

    chats[chat_id - 1].reactions[chats[chat_id - 1].num_reactions] = new_reaction;
    chats[chat_id - 1].num_reactions++;

    handle_get_chats(client_sock);  // Send updated chat list with reactions
}

void handle_reset(int client_sock) {
    free(chats); // Free dynamically allocated memory
    chats = NULL; // Set pointer to NULL
    chat_count = 0;  // Reset chat count
    // Don't send a response for reset to avoid showing "Server reset" in output
}

int main(int argc, char *argv[]) {
    int server_fd, client_sock, port = 30000;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size = sizeof(client_addr);

    // Get port from args, if provided
    if (argc > 1) {
        port = atoi(argv[1]);
    }

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Bind the socket
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_fd, 10) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server started on port %d\n", port);

    while (1) {
        client_sock = accept(server_fd, (struct sockaddr *)&client_addr, &addr_size);
        if (client_sock < 0) {
            perror("Connection failed");
            continue;
        }

        // Handle the request
        char buffer[1024];
        read(client_sock, buffer, sizeof(buffer) - 1);

        if (strstr(buffer, "/chats") != NULL) {
            handle_get_chats(client_sock);
        } else if (strstr(buffer, "/post") != NULL) {
            // Extract user and message parameters from query string
            char *user = strstr(buffer, "user=") + 5;
            char *message = strstr(buffer, "message=") + 8;
            if (user && message) {
                char user_str[50], message_str[256];
                sscanf(user, "%[^&]", user_str);
                sscanf(message, "%[^ ]", message_str);

                // Decode URL-encoded strings
                url_decode(user_str);
                url_decode(message_str);

                handle_post_message(client_sock, user_str, message_str);
            }
        } else if (strstr(buffer, "/react") != NULL) {
            char *user = strstr(buffer, "user=") + 5;
            char *reaction = strstr(buffer, "message=") + 8;
            char *id_str = strstr(buffer, "id=") + 3;
            int chat_id = atoi(id_str);
            if (user && reaction && chat_id > 0) {
                char user_str[50], reaction_str[256];
                sscanf(user, "%[^&]", user_str);
                sscanf(reaction, "%[^ ]", reaction_str);

                // Decode URL-encoded strings
                url_decode(user_str);
                url_decode(reaction_str);

                handle_post_reaction(client_sock, user_str, reaction_str, chat_id);
            }
        } else if (strstr(buffer, "/reset") != NULL) {
            handle_reset(client_sock); // No output for reset command
        }

        close(client_sock);
    }

    return 0;
}