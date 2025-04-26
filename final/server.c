#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "db.h"
#include "server.h"

int VERBOSE = 0;

void printf_debug(const char* format, ...) {
    if (VERBOSE) {
        va_list args;
        va_start(args, format);
        printf("\033[0;32m\t");
        vprintf(format, args);
        printf("\033[0m");   
        va_end(args);
    }
}

void* handle_client(void* arg) {

    // Extraer argumentos
    struct client_info* info = (struct client_info*)arg;
    int client_fd = info->client_fd;
    struct sockaddr_in client_addr = info->client_addr;
    free(info);

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);

    // Tratar cada comando
    char buffer[BUF_SIZE];
    int bytes_read = recv(client_fd, buffer, BUF_SIZE - 1, 0);
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';

        if (strncmp(buffer, "REGISTER", strlen("REGISTER")) == 0) {
            register_handler(client_fd, buffer);

        } else if (strncmp(buffer, "UNREGISTER", strlen("UNREGISTER")) == 0) {
            unregister_handler(client_fd, buffer);

        } else if (strncmp(buffer, "CONNECT", strlen("CONNECT")) == 0) {
            connect_handler(client_fd, buffer, client_ip);

        } else if (strncmp(buffer, "DISCONNECT", strlen("DISCONNECT")) == 0) {
            disconnect_handler(client_fd, buffer);

        } else if (strncmp(buffer, "PUBLISH", strlen("PUBLISH")) == 0) {
            publish_handler(client_fd, buffer);

        } else if (strncmp(buffer, "DELETE", strlen("DELETE")) == 0) {
            delete_handler(client_fd, buffer);

        } else if (strncmp(buffer, "LIST_USERS", strlen("LIST_USERS")) == 0) {
            list_users_handler(client_fd, buffer);

        } else if (strncmp(buffer, "LIST_CONTENT", strlen("LIST_CONTENT")) == 0) {
            list_content_handler(client_fd, buffer);

        } else {
            printf("s> unknown operation\n");
        }
    }

    close(client_fd);
    pthread_exit(NULL);
}


int main(int argc, char* argv[]) {
    int port;
    if (argc < 3) {
        fprintf(stderr, "uso: %s [-v] -p <puerto>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Parsear argumentos -v, -p
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0) {
            VERBOSE = 1;
        } else if (strcmp(argv[i], "-p") == 0) {
            if (i + 1 < argc) {
                port = atoi(argv[i + 1]);
                i++;
            } else {
                fprintf(stderr, "error: falta el número de puerto después de -p\n");
                exit(EXIT_FAILURE);
            }
        } else {
            fprintf(stderr, "uso: %s [-v] -p <puerto>\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    int server_fd;
    struct sockaddr_in server_addr;
    
    // Inicializar db
    db_init();

    // Socket TCP
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int optval = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));

    // Configurar dirección del servidor
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

    // Bucle multihilo
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


// ================== HANDLERS ====================


void register_handler(int client_fd, const char* buffer) {
    const char* datetime = buffer + strlen("REGISTER") + 1;
    const char* username = datetime + strlen(datetime) + 1;
    printf("s> OPERATION REGISTER FROM %s\n", username);

    int result = db_register_user(username);
    unsigned char response = result;

    switch (result) {
        case 0: printf_debug("OK\n"); break;
        case 1: printf_debug("USERNAME IN USE\n"); break;
        default:printf_debug("REGISTER FAIL\n"); break;
    }

    send(client_fd, &response, 1, 0);
}


void unregister_handler(int client_fd, const char* buffer) {
    const char* datetime = buffer + strlen("UNREGISTER") + 1;
    const char* username = datetime + strlen(datetime) + 1;
    printf("s> OPERATION UNREGISTER FROM %s\n", username);

    int result = db_unregister_user(username);
    unsigned char response = result;

    switch (result) {
        case 0: printf_debug("OK\n"); break;
        case 1: printf_debug("USERNAME DOES NOT EXIST\n"); break;
        default:printf_debug("UNREGISTER FAIL\n"); break;
    }

    send(client_fd, &response, 1, 0);
}


void connect_handler(int client_fd, const char* buffer, const char* client_ip) {
    const char* datetime = buffer + strlen("CONNECT") + 1;
    const char* username = datetime + strlen(datetime) + 1;
    const char* port_str = username + strlen(username) + 1;
    int port = atoi(port_str);

    printf("s> OPERATION CONNECT FROM %s\n", username);
    
    // validar puerto
    if (port <= 0) {
        unsigned char response = 3;
        send(client_fd, &response, 1, 0);
        printf_debug("(invalid port)\n");
        return;
    }

    int result = db_connect_user(username, port, client_ip);
    unsigned char response = result;

    switch (result) {
        case 0: printf_debug("OK\n"); break;
        case 1: printf_debug("USER DOES NOT EXIST\n"); break;
        case 2: printf_debug("USER ALREADY CONNECTED\n"); break;
        default:printf_debug("(unknown error)\n"); break;
    }

    send(client_fd, &response, 1, 0);
}


void disconnect_handler(int client_fd, const char* buffer) {
    const char* datetime = buffer + strlen("DISCONNECT") + 1;
    const char* username = datetime + strlen(datetime) + 1;
    printf("s> OPERATION DISCONNECT FROM %s\n", username);

    int result = db_disconnect_user(username);
    unsigned char response = result;

    switch (result) {
        case 0: printf_debug("OK\n"); break;
        case 1: printf_debug("USER DOES NOT EXIST\n"); break;
        case 2: printf_debug("USER NOT CONNECTED\n"); break;
        default:printf_debug("(unknown error) \n"); break;
    }

    send(client_fd, &response, 1, 0);
}


void publish_handler(int client_fd, const char* buffer) {
    const char* datetime = buffer + strlen("PUBLISH") + 1;
    const char* username = datetime + strlen(datetime) + 1;
    const char* filename = username + strlen(username) + 1;
    const char* description = filename + strlen(filename) + 1;

    printf("s> OPERATION PUBLISH FROM %s\n", username);

    int result = db_publish(username, filename, description);
    unsigned char response = result;

    switch (result) {
        case 0: printf_debug("OK\n"); break;
        case 1: printf_debug("USER DOES NOT EXIST\n"); break;
        case 2: printf_debug("USER NOT CONNECTED\n"); break;
        case 3: printf_debug("CONTENT ALREADY PUBLISHED (%s)\n", filename); break;
        default:printf_debug("(unknown error) \n"); break;
    }

    send(client_fd, &response, 1, 0);
}


void delete_handler(int client_fd, const char* buffer) {
    const char* datetime = buffer + strlen("DELETE") + 1;
    const char* username = datetime + strlen(datetime) + 1;
    const char* filename = username + strlen(username) + 1;

    printf("s> OPERATION DELETE FROM %s\n", username);

    int result = db_delete(username, filename);
    unsigned char response = result;

    switch (result) {
        case 0: printf_debug("OK\n"); break;
        case 1: printf_debug("USER DOES NOT EXIST\n"); break;
        case 2: printf_debug("USER NOT CONNECTED\n"); break;
        case 3: printf_debug("CONTENT NOT PUBLISHED: (%s)\n", filename); break;
        default:printf_debug("(unknown error) \n"); break;
    }

    send(client_fd, &response, 1, 0);
}



void list_users_handler(int client_fd, const char* buffer) {
    const char* datetime = buffer + strlen("LIST_USERS") + 1;
    const char* username = datetime + strlen(datetime) + 1;
    printf("s> OPERATION LIST_USERS FROM %s\n", username);

    user_entry_t** connected_users = NULL;
    int user_count = 0;
    int result = db_list_connected_users(username, &connected_users, &user_count);

    unsigned char response_code = result;
    send(client_fd, &response_code, 1, 0);

    switch (result) {
        case 0: {
            // Enviar número de usuarios como string terminado en '\0'
                char count_str[16];
                snprintf(count_str, sizeof(count_str), "%d", user_count);
                send(client_fd, count_str, strlen(count_str) + 1, 0);

                for (int i = 0; i < user_count; i++) {
                    // nombre, ip, puerto
                    send(client_fd, connected_users[i]->username, strlen(connected_users[i]->username) + 1, 0);
                    send(client_fd, connected_users[i]->ip, strlen(connected_users[i]->ip) + 1, 0);

                    char port_str[16];
                    snprintf(port_str, sizeof(port_str), "%d", connected_users[i]->listening_port);
                    send(client_fd, port_str, strlen(port_str) + 1, 0);
                }

                free(connected_users);
                printf_debug("OK\n");
            }
            break;
        case 1: printf_debug("USER DOES NOT EXIST\n"); break;
        case 2: printf_debug("USER NOT CONNECTED\n"); break;
        default:printf_debug("(unknown error) \n"); break;
    }
}



void list_content_handler(int client_fd, const char* buffer) {
    const char* datetime = buffer + strlen("LIST_CONTENT") + 1;
    const char* username = datetime + strlen(datetime) + 1;
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

        // file name y descripcion (de regalo)
        for (int i = 0; i < file_count; i++) {
            send(client_fd, files[i]->filename, strlen(files[i]->filename) + 1, 0);
            send(client_fd, files[i]->description, strlen(files[i]->description) + 1, 0);
        }

        free(files);
        printf_debug("LIST_CONTENT OK for %s\n", remote_username);

    } else {
        response = result;

        send(client_fd, &response, 1, 0);
        printf_debug("LIST_CONTENT FAIL for %s (code %d)\n", username, result);
    }
}

