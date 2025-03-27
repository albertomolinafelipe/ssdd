#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "claves.h"

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


void pack_request(int socket, const message_t *msg) {
    if (!msg) return;

    char buffer[256];

    // command
    sprintf(buffer, "%d", msg->cmd);
    send_message(socket, buffer, strlen(buffer) + 1);

    // sender_tid
    sprintf(buffer, "%lu", msg->sender_tid);
    send_message(socket, buffer, strlen(buffer) + 1);

    // type
    sprintf(buffer, "%d", msg->type);
    send_message(socket, buffer, strlen(buffer) + 1);

    // key (if not destroy)
    if (msg->cmd != CMD_TYPE_DESTROY) {
        sprintf(buffer, "%d", msg->key);
        send_message(socket, buffer, strlen(buffer) + 1);
    }

    // values (only on set and destroy)
    if (msg->cmd == CMD_TYPE_SET || msg->cmd == CMD_TYPE_MODIFY) {
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

int unpack_response(int socket, message_t *msg) {
    if (!msg) return -1;

    char buffer[256];

    // command
    if (read_line(socket, buffer, sizeof(buffer)) <= 0) return -1;
    msg->cmd = atoi(buffer);

    // result
    if (read_line(socket, buffer, sizeof(buffer)) <= 0) return -1;
    msg->result = atoi(buffer);

    // sender_tid
    if (read_line(socket, buffer, sizeof(buffer)) <= 0) return -1;
    msg->sender_tid = strtoul(buffer, NULL, 10);

    // type
    if (read_line(socket, buffer, sizeof(buffer)) <= 0) return -1;
    msg->type = atoi(buffer);

    // values only if get command
    if (msg->cmd == CMD_TYPE_GET && msg->result == 0) {
        if (read_line(socket, buffer, sizeof(buffer)) <= 0) return -1;
        msg->key = atoi(buffer);

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



int make_request(message_t *msg) {
    // Connection params
    const char *server_ip = getenv("IP_TUPLAS");
    const char *server_port_str = getenv("PORT_TUPLAS");
    if (!server_ip || !server_port_str) {
        fprintf(stderr, "Error: Environment variable not set\n");
        return -2;
    }

    int server_port = atoi(server_port_str);
    if (server_port <= 0) {
        fprintf(stderr, "Error: Invalid PORT_TUPLAS\n");
        return -2;
    }

    // Prepare request message
    msg->sender_tid = pthread_self();
    msg->type = MSG_TYPE_REQUEST;

    // Create socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return -2;
    }

    struct sockaddr_in server_addr;
    struct hostent *hp = gethostbyname(server_ip);
    if (!hp) {
        perror("gethostbyname");
        close(sock);
        return -2;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    memcpy(&server_addr.sin_addr, hp->h_addr, hp->h_length);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);

    // Connect to server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sock);
        return -2;
    }

    // send
    pack_request(sock, msg);

    // receive
    message_t response_msg;
    if (unpack_response(sock, &response_msg) < 0) {
        close(sock);
        return -2;
    }

    close(sock);

    // verify response
    if (response_msg.sender_tid != msg->sender_tid) {
        fprintf(stderr, "Received response for another request\n");
        return -2;
    }

    // copy results if it was a get command
    msg->result = response_msg.result;
    if (msg->cmd == CMD_TYPE_GET && msg->result == 0) {
        strncpy(msg->value1, response_msg.value1, sizeof(msg->value1) - 1);
        msg->value1[sizeof(msg->value1) - 1] = '\0';
        msg->N_value2 = response_msg.N_value2;
        memcpy(msg->V_value2, response_msg.V_value2, sizeof(msg->V_value2));
        msg->value3 = response_msg.value3;
    }

    return msg->result;
}


int destroy() {
    message_t msg = {
        .cmd = CMD_TYPE_DESTROY
    };
    int err = make_request(&msg);
    return err;
}

int set_value(int key, char *value1, int N_value2, double *V_value2, struct Coord value3) {
    // Check format
    if (strlen(value1) > 255) return -1;
    if (N_value2 < 1 || N_value2 > 32) return -1;

    message_t msg = {
        .cmd = CMD_TYPE_SET,
        .key = key,
        .N_value2 = N_value2,
        .value3 = value3,
    };
    // Copy values into msg
    // Limit string
    strncpy(msg.value1, value1, sizeof(msg.value1) - 1);
    msg.value1[sizeof(msg.value1) - 1] = '\0';

    memcpy(msg.V_value2, V_value2, msg.N_value2 * sizeof(double));
    return make_request(&msg);
}

int get_value(int key, char *value1, int *N_value2, double *V_value2, struct Coord *value3) {
    message_t msg = {
        .cmd = CMD_TYPE_GET,
        .key = key
    };

    int res = make_request(&msg);
    // on success copy values
    if (res == 0) {
        strncpy(value1, msg.value1, sizeof(msg.value1) - 1);
        *N_value2 = msg.N_value2;
        memcpy(V_value2, msg.V_value2, msg.N_value2 * sizeof(double));
        *value3 = msg.value3;
    }
    return res;
}

int modify_value(int key, char *value1, int N_value2, double *V_value2, struct Coord value3) {
    // Check format
    if (strlen(value1) > 255) return -1;
    if (N_value2 < 1 || N_value2 > 32) return -1;

    message_t msg = {
        .cmd = CMD_TYPE_MODIFY,
        .key = key,
        .N_value2 = N_value2,
        .value3 = value3,
    };
    // Copy values into msg
    // Limit string
    strncpy(msg.value1, value1, sizeof(msg.value1) - 1);
    msg.value1[sizeof(msg.value1) - 1] = '\0';

    memcpy(msg.V_value2, V_value2, msg.N_value2 * sizeof(double));
    return make_request(&msg);
}

int delete_key(int key) {
    message_t msg = {
        .cmd = CMD_TYPE_DELETE,
        .key = key,
    };
    return make_request(&msg);
}

int exist(int key) {
    message_t msg = {
        .cmd = CMD_TYPE_EXIST,
        .key = key,
    };
    return make_request(&msg);
}
