#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include "claves.h"

#define MAX_CONN 10
int VERBOSE = 0;

int busy = 0 ;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER ;
pthread_cond_t  c = PTHREAD_COND_INITIALIZER ;

ssize_t send_message(int fd, const void *buffer, size_t n) {
    if (!buffer || n == 0) return -1;
    return write(fd, buffer, n);
}

ssize_t read_line(int fd, void *buffer, size_t n) {
    ssize_t numRead; /* Number of bytes fetched by last read() */
    size_t totRead;   /* Total bytes read so far */
    char *buf;
    char ch;

    /* Check for invalid input */
    if (n <= 0 || buffer == NULL) {
        errno = EINVAL;
        return -1;
    }

    buf = buffer;
    totRead = 0;

    /* Loop to read characters until newline, null terminator, EOF, or error */
    for (;;) {
        numRead = read(fd, &ch, 1); /* Read a single byte */

        if (numRead == -1) {
            /* Error occurred during read */
            if (errno == EINTR) {
                /* Interrupted system call, retry read */
                continue;
            } else {
                /* Other error, return -1 */
                return -1;
            }
        } else if (numRead == 0) {
            /* End of file (EOF) reached */
            if (totRead == 0) {
                /* No bytes read before EOF, return 0 */
                return 0;
            } else {
                /* Bytes read before EOF, break out of the loop */
                break;
            }
        } else {
            /* Successfully read one byte */
            if (ch == '\n' || ch == '\0') {
                /* Newline or null terminator encountered, break out of the loop */
                break;
            }

            /* Check if buffer has space for the character */
            if (totRead < n - 1) {
                /* Store the character in the buffer */
                totRead++;
                *buf++ = ch;
            }
            /* If totRead >= n-1, the character is discarded */
        }
    }

    /* Null-terminate the buffer */
    *buf = '\0';

    /* Return the total number of bytes read */
    return totRead;
}


void pack_response(int socket, const message_t *msg) {
    if (!msg) return;

    char buffer[256];

    // command
    sprintf(buffer, "%d", msg->cmd);
    send_message(socket, buffer, strlen(buffer) + 1);

    // result
    sprintf(buffer, "%d", msg->result);
    send_message(socket, buffer, strlen(buffer) + 1);

    // sender_tid
    sprintf(buffer, "%lu", msg->sender_tid);
    send_message(socket, buffer, strlen(buffer) + 1);

    // type
    sprintf(buffer, "%d", msg->type);
    send_message(socket, buffer, strlen(buffer) + 1);

    // values only on get
    if (msg->cmd == CMD_TYPE_GET && msg->result == 0) {
        sprintf(buffer, "%d", msg->key);
        send_message(socket, buffer, strlen(buffer) + 1);

        send_message(socket, msg->value1, strlen(msg->value1) + 1);

        sprintf(buffer, "%d", msg->N_value2);
        send_message(socket, buffer, strlen(buffer) + 1);

        for (int i = 0; i < msg->N_value2; i++) {
            sprintf(buffer, "%f", msg->V_value2[i]);
            send_message(socket, buffer, strlen(buffer) + 1);
        }

        sprintf(buffer, "%d", msg->value3.x);
        send_message(socket, buffer, strlen(buffer) + 1);

        sprintf(buffer, "%d", msg->value3.y);
        send_message(socket, buffer, strlen(buffer) + 1);
    }
}



int unpack_request(int socket, message_t *msg) {
    if (!msg) return -1;

    char buffer[256];

    // command
    if (read_line(socket, buffer, sizeof(buffer)) <= 0) return -1;
    msg->cmd = atoi(buffer);

    // sender_tid
    if (read_line(socket, buffer, sizeof(buffer)) <= 0) return -1;
    msg->sender_tid = strtoul(buffer, NULL, 10);

    // type
    if (read_line(socket, buffer, sizeof(buffer)) <= 0) return -1;
    msg->type = atoi(buffer);

    // key (if not destroy)
    if (msg->cmd != CMD_TYPE_DESTROY) {
        if (read_line(socket, buffer, sizeof(buffer)) <= 0) return -1;
        msg->key = atoi(buffer);
    }

    // values (only set and destroy)
    if (msg->cmd == CMD_TYPE_SET || msg->cmd == CMD_TYPE_MODIFY) {
        if (read_line(socket, msg->value1, sizeof(msg->value1)) <= 0) return -1;

        if (read_line(socket, buffer, sizeof(buffer)) <= 0) return -1;
        msg->N_value2 = atoi(buffer);

        for (int i = 0; i < msg->N_value2 && i < 32; i++) {
            if (read_line(socket, buffer, sizeof(buffer)) <= 0) return -1;
            msg->V_value2[i] = strtod(buffer, NULL);
        }

        if (read_line(socket, buffer, sizeof(buffer)) <= 0) return -1;
        msg->value3.x = atoi(buffer);

        if (read_line(socket, buffer, sizeof(buffer)) <= 0) return -1;
        msg->value3.y = atoi(buffer);
    }

    return 0;
}



message_t get_response(message_t *msg) {
    message_t response;
    int result = 0;

    switch (msg->cmd) {
        case CMD_TYPE_DESTROY:
            result = destroy();
            break;
        case CMD_TYPE_SET:
            result = set_value(
                    msg->key,
                    msg->value1,
                    msg->N_value2,
                    msg->V_value2,
                    msg->value3);
            break;
        case CMD_TYPE_GET:
            result = get_value(
                    msg->key,
                    response.value1,
                    &response.N_value2,
                    response.V_value2,
                    &response.value3);
            break;
        case CMD_TYPE_MODIFY:
            result = modify_value(
                    msg->key,
                    msg->value1,
                    msg->N_value2,
                    msg->V_value2,
                    msg->value3);
            break;
        case CMD_TYPE_DELETE:
            result = delete_key(msg->key);
            break;
        case CMD_TYPE_EXIST:
            result = exist(msg->key);
            break;
        default:
            break;
    }

    response.result = result;
    response.sender_tid = msg->sender_tid;
    response.cmd = msg->cmd;
    return response;
}

// Function to handle client requests
void *handle_request(void *arg) {
    int client_sock;

    // Synchronization: Mark thread as busy and get client socket
    pthread_mutex_lock(&m);
    busy = 0;
    client_sock = *((int *)arg);
    pthread_cond_signal(&c);
    pthread_mutex_unlock(&m);

    // Receive the request
    message_t msg;
    if (unpack_request(client_sock, &msg) < 0) {
        fprintf(stderr, "Error: Failed to unpack request\n");
        close(client_sock);
        return NULL;
    }

    // Handle the request
    message_t response = get_response(&msg);
    response.type = MSG_TYPE_RESPONSE;

    // Print messages for debugging (if VERBOSE is enabled)
    print_msg(&msg, VERBOSE);
    print_msg(&response, VERBOSE);
    fflush(stdout);

    // Send the response
    pack_response(client_sock, &response);

    close(client_sock);
    pthread_exit(NULL);
    return NULL;
}



int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <port> [--verbose|-v]\n", argv[0]);
        return 1;
    }

    if (argc > 2 && (strcmp(argv[2], "--verbose") == 0 || strcmp(argv[2], "-v") == 0)) {
        VERBOSE = 1;
    }

    int port = atoi(argv[1]);
    if (port <= 0) {
        fprintf(stderr, "Invalid port number: %s\n", argv[1]);
        return 1;
    }

    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("socket");
        return 1;
    }

    socklen_t size;
    struct sockaddr_in server_addr, client_addr;
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_sock);
        return 1;
    }

    if (listen(server_sock, MAX_CONN) < 0) {
        perror("listen");
        close(server_sock);
        return 1;
    }

    printf("SERVER THREAD: %lu\n", pthread_self());
    printf("Listening on port %d...\n", port);

    pthread_attr_t attr ;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    while (1) {
        size = sizeof(struct sockaddr_in) ;
        bzero(&client_addr, size);
        int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &size);
        if (client_sock < 0) {
            perror("accept");
            return -1;
        };

        // Create and detach the thread for each client request
        pthread_t thread;
        pthread_create(&thread, &attr, handle_request, (void *)&client_sock);
        pthread_mutex_lock(&m);
        while (busy == 1) {
            pthread_cond_wait(&c, &m);
        }
        busy = 1;
        pthread_mutex_unlock(&m);
    }
    close(server_sock);
    return 0;
}
