#include "http-server.h"

void start_server(void (*handler)(char *, int), int port) {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    int enable = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    if (listen(server_sock, 10) < 0) {
        perror("Listen failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    socklen_t addr_len = sizeof(server_addr);
    if (getsockname(server_sock, (struct sockaddr *)&server_addr, &addr_len) == -1) {
        perror("getsockname failed");
        exit(EXIT_FAILURE);
    }

    printf("Server started on port %d\n", ntohs(server_addr.sin_port));

    char buffer[BUFFER_SIZE];
    while (1) {
        if ((client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len)) < 0) {
            perror("Accept failed");
            continue;
        }

        int bytes = recv(client_sock, buffer, BUFFER_SIZE - 1, 0);
        if (bytes < 0) {
            perror("Receive failed");
            close(client_sock);
            continue;
        }
        buffer[bytes] = '\0';
        handler(buffer, client_sock);

        close(client_sock);
    }

    close(server_sock);
}