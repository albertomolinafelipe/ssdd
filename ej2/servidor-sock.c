#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cjson/cJSON.h>
#include <fcntl.h>
#include "claves.h"

#define MAX_CONN 10
int VERBOSE = 0;

int busy = 0 ;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER ;
pthread_cond_t  c = PTHREAD_COND_INITIALIZER ;

char *pack_response(const message_t *msg) {
    if (!msg) return NULL;

    cJSON *json = cJSON_CreateObject();
    if (!json) return NULL;

    cJSON_AddNumberToObject(json, "cmd", msg->cmd);
    cJSON_AddNumberToObject(json, "result", msg->result);
    cJSON_AddNumberToObject(json, "sender_tid", msg->sender_tid);
    cJSON_AddNumberToObject(json, "type", msg->type);

    if (msg->cmd == CMD_TYPE_GET && msg->result == 0) {
        cJSON_AddNumberToObject(json, "key", msg->key);

        cJSON_AddStringToObject(json, "value1", msg->value1);

        cJSON_AddNumberToObject(json, "N_value2", msg->N_value2);
        cJSON *array = cJSON_CreateArray();
        if (!array) {
            cJSON_Delete(json);
            return NULL;
        }

        for (int i = 0; i < msg->N_value2; i++) {
            cJSON_AddItemToArray(array, cJSON_CreateNumber(msg->V_value2[i]));
        }
        cJSON_AddItemToObject(json, "V_value2", array);

        cJSON *coord = cJSON_CreateObject();
        if (!coord) {
            cJSON_Delete(json);
            return NULL;
        }

        cJSON_AddNumberToObject(coord, "x", msg->value3.x);
        cJSON_AddNumberToObject(coord, "y", msg->value3.y);
        cJSON_AddItemToObject(json, "value3", coord);
    }

    char *json_str = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);

    return json_str;
}

int unpack_request(const char *json_str, message_t *msg) {
    if (!json_str || !msg) return -1;

    cJSON *json = cJSON_Parse(json_str);
    if (!json) return -1;

    cJSON *cmd = cJSON_GetObjectItem(json, "cmd");
    cJSON *sender_tid = cJSON_GetObjectItem(json, "sender_tid");
    cJSON *type = cJSON_GetObjectItem(json, "type");
    
    if (!cmd || !sender_tid || !type) {
        cJSON_Delete(json);
        return -1;
    }

    msg->cmd = cmd->valueint;
    msg->sender_tid = sender_tid->valuedouble;
    msg->type = type->valueint;

    cJSON *key = (msg->cmd != CMD_TYPE_DESTROY) ? cJSON_GetObjectItem(json, "key") : NULL;
    int unpack_values = (msg->cmd == CMD_TYPE_SET || msg->cmd == CMD_TYPE_MODIFY);
    cJSON *value1 = unpack_values ? cJSON_GetObjectItem(json, "value1") : NULL;
    cJSON *N_value2 = unpack_values ? cJSON_GetObjectItem(json, "N_value2") : NULL;
    cJSON *V_value2 = unpack_values ? cJSON_GetObjectItem(json, "V_value2") : NULL;
    cJSON *value3 = unpack_values ? cJSON_GetObjectItem(json, "value3") : NULL;

    if ((msg->cmd != CMD_TYPE_DESTROY && !key) ||
        unpack_values && (!value1 || !N_value2 || !V_value2 || !value3)) {
        cJSON_Delete(json);
        return -1;
    }

    if (key) {
        msg->key = key->valueint;
    }
    
    if (value1) {
        strncpy(msg->value1, value1->valuestring, sizeof(msg->value1) - 1);
        msg->value1[sizeof(msg->value1) - 1] = '\0';
    }

    if (N_value2) {
        msg->N_value2 = N_value2->valueint;
        for (int i = 0; i < msg->N_value2 && i < 32; i++) {
            cJSON *item = cJSON_GetArrayItem(V_value2, i);
            if (item) {
                msg->V_value2[i] = item->valuedouble;
            }
        }
    }

    if (value3) {
        cJSON *x = cJSON_GetObjectItem(value3, "x");
        cJSON *y = cJSON_GetObjectItem(value3, "y");
        if (x && y) {
            msg->value3.x = x->valuedouble;
            msg->value3.y = y->valuedouble;
        }
    }
    
    cJSON_Delete(json);
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
    pthread_mutex_lock(&m);
    busy = 0;
    client_sock = (* (int *) arg);
    pthread_cond_signal(&c);
    pthread_mutex_unlock(&m);
    
    char buffer[BUFFER_SIZE];
    
    // Receive JSON message from the client
    int recv_len = recv(client_sock, buffer, sizeof(buffer), 0);
    if (recv_len < 0) {
        perror("recv");
        close(client_sock);
        return NULL;
    }
    buffer[recv_len] = '\0';

    // Parse the received JSON message
    cJSON *json = cJSON_Parse(buffer);
    if (!json) {
        perror("JSON parsing failed");
        close(client_sock);
        return NULL;
    }

    // Convert JSON to message_t structure
    message_t msg;
    char *json_str = cJSON_PrintUnformatted(json);
    unpack_request(json_str, &msg);
    free(json_str);
    cJSON_Delete(json); // Free the JSON object

    // Handle the request
    message_t response = get_response(&msg);
    response.type = MSG_TYPE_RESPONSE;
    print_msg(&msg, VERBOSE);
    print_msg(&response, VERBOSE);
    fflush(stdout);

    // Convert the response message to JSON
    char *response_json = pack_response(&response);

    // Send the response JSON back to the client
    if (send(client_sock, response_json, strlen(response_json), 0) < 0) {
        perror("send");
    }

    free(response_json);
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
