#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "db.h"

#define BACKLOG 10
#define BUF_SIZE 512

struct client_info {
    int client_fd;
    struct sockaddr_in client_addr;
};

void* handle_client(void* arg);
void register_handler(int client_fd, const char* username);
void unregister_handler(int client_fd, const char* username);
void connect_handler(int client_fd, const char* buffer, const char* client_ip);
void disconnect_handler(int client_fd, const char* username);
void publish_handler(int client_fd, const char* buffer);
void delete_handler(int client_fd, const char* buffer) ;
void list_users_handler(int client_fd, const char* username) ;
void list_content_handler(int client_fd, const char* buffer) ;

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

        struct client_info* info = malloc(sizeof(struct client_info));
        info->client_fd = client_fd;
        info->client_addr = client_addr;
        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, info);
        pthread_detach(tid);
    }

    close(server_fd);
    db_destroy();
    return 0;
}

void* handle_client(void* arg) {
    struct client_info* info = (struct client_info*)arg;
    int client_fd = info->client_fd;
    struct sockaddr_in client_addr = info->client_addr;
    free(info);

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);

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
            connect_handler(client_fd, buffer, client_ip);

        } else if (strncmp(buffer, "DISCONNECT", 10) == 0) {
            char* username = buffer + strlen("DISCONNECT") + 1;
            disconnect_handler(client_fd, username);

        } else if (strncmp(buffer, "PUBLISH", 7) == 0) {
            publish_handler(client_fd, buffer);

        } else if (strncmp(buffer, "DELETE", 6) == 0) {
            delete_handler(client_fd, buffer);

        } else if (strncmp(buffer, "LIST_USERS", 10) == 0) {
            char* username = buffer + strlen("LIST_USERS") + 1;
            list_users_handler(client_fd, username);

        } else if (strncmp(buffer, "LIST_CONTENT", 12) == 0) {
            list_content_handler(client_fd, buffer);

        } else {
            printf("s> unknown operation\n");
        }
    }

    close(client_fd);
    pthread_exit(NULL);
}

void register_handler(int client_fd, const char* username) {
    printf("s> OPERATION REGISTER FROM %s\n", username);
    int result = db_register_user(username);

    unsigned char response;
    if (result == 0) {
        response = 0;
        // printf("s> REGISTER OK\n");
    } else if (result == 1) {
        response = 1;
        // printf("s> USERNAME IN USE\n");
    } else {
        response = 2;
        // printf("s> REGISTER FAIL\n");
    }

    send(client_fd, &response, 1, 0);
}

void unregister_handler(int client_fd, const char* username) {
    printf("s> OPERATION UNREGISTER FROM %s\n", username);
    int result = db_unregister_user(username);

    unsigned char response;
    if (result == 0) {
        response = 0;
        // printf("s> UNREGISTER OK\n");
    } else if (result == 1) {
        response = 1;
        // printf("s> USERNAME DOES NOT EXIST\n");
    } else {
        response = 2;
        // printf("s> UNREGISTER FAIL\n");
    }

    send(client_fd, &response, 1, 0);
}



void connect_handler(int client_fd, const char* buffer, const char* client_ip) {
    const char* username = buffer + strlen("CONNECT") + 1;
    printf("s> OPERATION CONNECT FROM %s\n", username);
    const char* port_str = username + strlen(username) + 1;
    int port = atoi(port_str);

    if (port <= 0) {
        unsigned char response = 3;
        send(client_fd, &response, 1, 0);
        // printf("s> CONNECT FAIL (invalid port) from %s\n", username);
        return;
    }


    int result = db_connect_user(username, port, client_ip);

    unsigned char response;
    if (result == 0) {
        response = 0;
        // printf("s> CONNECT OK for %s\n", username);
    } else if (result == 1) {
        response = 1;
        // printf("s> CONNECT FAIL, USER DOES NOT EXIST: %s\n", username);
    } else if (result == 2) {
        response = 2;
        // printf("s> USER ALREADY CONNECTED: %s\n", username);
    } else {
        response = 3;
        // printf("s> CONNECT FAIL (unknown error) for %s\n", username);
    }

    send(client_fd, &response, 1, 0);
}



void disconnect_handler(int client_fd, const char* username) {
    printf("s> OPERATION DISCONNECT FROM %s\n", username);
    int result = db_disconnect_user(username);

    unsigned char response;
    if (result == 0) {
        response = 0;
        // printf("s> DISCONNECT OK FOR %s\n", username);
    } else if (result == 1) {
        response = 1;
        // printf("s> DISCONNECT FAIL, USER DOES NOT EXIST: %s\n", username);
    } else if (result == 2) {
        response = 2;
        // printf("s> DISCONNECT FAIL, USER NOT CONNECTED: %s\n", username);
    } else {
        response = 3;
        // printf("s> DISCONNECT FAIL (UNKNOWN ERROR) FOR %s\n", username);
    }

    send(client_fd, &response, 1, 0);
}



void publish_handler(int client_fd, const char* buffer) {
    const char* username = buffer + strlen("PUBLISH") + 1;
    printf("s> OPERATION PUBLISH FROM %s\n", username);
    const char* filename = username + strlen(username) + 1;
    const char* description = filename + strlen(filename) + 1;


    int result = db_publish(username, filename, description);

    unsigned char response;
    if (result == 0) {
        response = 0;
        // printf("s> PUBLISH OK for user %s, file %s\n", username, filename);
    } else if (result == 1) {
        response = 1;
        // printf("s> PUBLISH FAIL, USER DOES NOT EXIST: %s\n", username);
    } else if (result == 2) {
        response = 2;
        // printf("s> PUBLISH FAIL, USER NOT CONNECTED: %s\n", username);
    } else if (result == 3) {
        response = 3;
        // printf("s> PUBLISH FAIL, CONTENT ALREADY PUBLISHED: %s (%s)\n", username, filename);
    } else {
        response = 4;
        // printf("s> PUBLISH FAIL (unknown error) for %s\n", username);
    }

    send(client_fd, &response, 1, 0);
}


void delete_handler(int client_fd, const char* buffer) {
    const char* username = buffer + strlen("DELETE") + 1;
    printf("s> OPERATION DELETE FROM %s\n", username);
    const char* filename = username + strlen(username) + 1;


    int result = db_delete(username, filename);

    unsigned char response;
    if (result == 0) {
        response = 0;
        // printf("s> DELETE OK for user %s, file %s\n", username, filename);
    } else if (result == 1) {
        response = 1;
        // printf("s> DELETE FAIL, USER DOES NOT EXIST: %s\n", username);
    } else if (result == 2) {
        response = 2;
        // printf("s> DELETE FAIL, USER NOT CONNECTED: %s\n", username);
    } else if (result == 3) {
        response = 3;
        // printf("s> DELETE FAIL, CONTENT NOT PUBLISHED: %s (%s)\n", username, filename);
    } else {
        response = 4;
        // printf("s> DELETE FAIL (unknown error) for %s\n", username);
    }

    send(client_fd, &response, 1, 0);
}

void list_content_handler(int client_fd, const char* buffer) {
    const char* username = buffer + strlen("LIST_CONTENT") + 1;
    printf("s> OPERATION LIST_CONTENT FROM %s\n", username);
    const char* remote_username = username + strlen(username) + 1;


    file_entry_t** files = NULL;
    int file_count = 0;

    int result = db_list_user_content(username, remote_username, &files, &file_count);
    unsigned char response;

    if (result == 0) {
        response = 0;
        send(client_fd, &response, 1, 0);

        // Enviar el número de ficheros publicados como string terminado en '\0'
        char count_str[16];
        snprintf(count_str, sizeof(count_str), "%d", file_count);
        send(client_fd, count_str, strlen(count_str) + 1, 0);

        // Enviar filename y description de cada fichero
        for (int i = 0; i < file_count; i++) {
            send(client_fd, files[i]->filename, strlen(files[i]->filename) + 1, 0);
            send(client_fd, files[i]->description, strlen(files[i]->description) + 1, 0);
        }

        free(files);
        // printf("s> LIST_CONTENT OK for %s\n", remote_username);

    } else {
        if (result == 1) response = 1;  // USER DOES NOT EXIST
        else if (result == 2) response = 2;  // USER NOT CONNECTED
        else if (result == 3) response = 3;  // REMOTE USER DOES NOT EXIST
        else response = 4;  // UNKNOWN ERROR

        send(client_fd, &response, 1, 0);
        // printf("s> LIST_CONTENT FAIL for %s (code %d)\n", username, result);
    }
}

void list_users_handler(int client_fd, const char* username) {
    printf("s> OPERATION LIST_USERS FROM %s\n", username);
    user_entry_t** connected_users = NULL;
    int user_count = 0;
    int result = db_list_connected_users(username, &connected_users, &user_count);

    unsigned char response_code;

    if (result == 0) {
        response_code = 0;
        send(client_fd, &response_code, 1, 0);

        // Enviar el número de usuarios como string terminado en '\0'
        char count_str[16];
        snprintf(count_str, sizeof(count_str), "%d", user_count);
        send(client_fd, count_str, strlen(count_str) + 1, 0);  // +1 para enviar el '\0'

        for (int i = 0; i < user_count; i++) {
            send(client_fd, connected_users[i]->username, strlen(connected_users[i]->username) + 1, 0);

            // IP: si no tienes IP real, usa "0.0.0.0"
            send(client_fd, "0.0.0.0", strlen("0.0.0.0") + 1, 0);

            char port_str[16];
            snprintf(port_str, sizeof(port_str), "%d", connected_users[i]->listening_port);
            send(client_fd, port_str, strlen(port_str) + 1, 0);
        }

        free(connected_users);
        // printf("s> LIST_USERS OK for %s\n", username);
    } else  {
        response_code = result;
        send(client_fd, &response_code, 1, 0);
    } 
}
