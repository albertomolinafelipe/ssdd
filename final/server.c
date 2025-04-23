#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "db.h"

#define BACKLOG 10
#define BUF_SIZE 512

void* handle_client(void* arg);
void register_handler(int client_fd, const char* username);
void unregister_handler(int client_fd, const char* username);
void connect_handler(int client_fd, const char* username);
void disconnect_handler(int client_fd, const char* username);
void publish_handler(int client_fd);
void delete_handler(int client_fd);
void list_users_handler(int client_fd);
void list_content_handler(int client_fd);

int main(int argc, char* argv[]) {
    if (argc != 3 || strcmp(argv[1], "-p") != 0) {
        fprintf(stderr, "uso: %s -p <puerto>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[2]);
    int server_fd;
    struct sockaddr_in server_addr;

    db_init();

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int optval = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(server_addr.sin_zero), '\0', 8);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr)) == -1) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, BACKLOG) == -1) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("s> init server 0.0.0.0:%d\n", port);
    printf("s>\n");

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t sin_size = sizeof(struct sockaddr_in);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &sin_size);
        if (client_fd == -1) {
            perror("accept");
            continue;
        }

        int* pclient = malloc(sizeof(int));
        *pclient = client_fd;

        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, pclient);
        pthread_detach(tid);
    }

    close(server_fd);
    db_destroy();
    return 0;
}

void* handle_client(void* arg) {
    int client_fd = *((int*)arg);
    free(arg);

    char buffer[BUF_SIZE];
    int bytes_read = recv(client_fd, buffer, BUF_SIZE - 1, 0);
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';

        if (strncmp(buffer, "REGISTER", 8) == 0) {
            char* username = buffer + strlen("REGISTER") + 1;
            register_handler(client_fd, username);

        } else if (strncmp(buffer, "UNREGISTER", 10) == 0) {
            char* username = buffer + strlen("UNREGISTER") + 1;
            unregister_handler(client_fd, username);

        } else if (strncmp(buffer, "CONNECT", 7) == 0) {
            connect_handler(client_fd, buffer);

        } else if (strncmp(buffer, "DISCONNECT", 10) == 0) {

        } else if (strncmp(buffer, "PUBLISH", 7) == 0) {

        } else if (strncmp(buffer, "DELETE", 6) == 0) {

        } else if (strncmp(buffer, "LIST USERS", 10) == 0) {

        } else if (strncmp(buffer, "LIST CONTENT", 12) == 0) {

        } else {
            printf("s> unknown operation\n");
        }
    }

    close(client_fd);
    pthread_exit(NULL);
}

void register_handler(int client_fd, const char* username) {
    int result = db_register_user(username);

    unsigned char response;
    if (result == 0) {
        response = 0;
        printf("s> REGISTER OK\n");
    } else if (result == 1) {
        response = 1;
        printf("s> USERNAME IN USE\n");
    } else {
        response = 2;
        printf("s> REGISTER FAIL\n");
    }

    send(client_fd, &response, 1, 0);
}

void unregister_handler(int client_fd, const char* username) {
    int result = db_unregister_user(username);

    unsigned char response;
    if (result == 0) {
        response = 0;
        printf("s> UNREGISTER OK\n");
    } else if (result == 1) {
        response = 1;
        printf("s> USERNAME DOES NOT EXIST\n");
    } else {
        response = 2;
        printf("s> UNREGISTER FAIL\n");
    }

    send(client_fd, &response, 1, 0);
}


void connect_handler(int client_fd, const char* buffer) {
    const char* username = buffer + strlen("CONNECT") + 1;
    const char* port_str = username + strlen(username) + 1;

    int port = atoi(port_str);
    if (port <= 0) {
        unsigned char response = 3;
        send(client_fd, &response, 1, 0);
        printf("s> CONNECT FAIL (invalid port) from %s\n", username);
        return;
    }

    printf("s> OPERATION CONNECT FROM %s (port %d)\n", username, port);

    int result = db_connect_user(username, port);

    unsigned char response;
    if (result == 0) {
        response = 0;
        printf("s> CONNECT OK for %s\n", username);
    } else if (result == 1) {
        response = 1;
        printf("s> CONNECT FAIL, USER DOES NOT EXIST: %s\n", username);
    } else if (result == 2) {
        response = 2;
        printf("s> USER ALREADY CONNECTED: %s\n", username);
    } else {
        response = 3;
        printf("s> CONNECT FAIL (unknown error) for %s\n", username);
    }

    send(client_fd, &response, 1, 0);
}


void disconnect_handler(int client_fd, const char* username) {
    // TODO: IMPLEMENTAR
}

void publish_handler(int client_fd) {
    // TODO: IMPLEMENTAR
}

void delete_handler(int client_fd) {
    // TODO: IMPLEMENTAR
}

void list_users_handler(int client_fd) {
    // TODO: IMPLEMENTAR
}

void list_content_handler(int client_fd) {
    // TODO: IMPLEMENTAR
}
