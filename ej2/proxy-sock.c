#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cjson/cJSON.h>
#include "claves.h"

char *pack_request(const message_t *msg) {
    if (!msg) return NULL;

    cJSON *json = cJSON_CreateObject();
    if (!json) return NULL;

    // always
    cJSON_AddNumberToObject(json, "cmd", msg->cmd);
    cJSON_AddNumberToObject(json, "sender_tid", msg->sender_tid);
    cJSON_AddNumberToObject(json, "type", msg->type);
    
    // no key on destroy
    if (msg->cmd != CMD_TYPE_DESTROY) {
        cJSON_AddNumberToObject(json, "key", msg->key);
    }
    
    // only values on set and modify
    if (msg->cmd == CMD_TYPE_SET || msg->cmd == CMD_TYPE_MODIFY) {
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

int unpack_response(const char *json_str, message_t *msg) {
    if (!json_str || !msg) return -1;

    cJSON *json = cJSON_Parse(json_str);
    if (!json) return -1;

    cJSON *cmd = cJSON_GetObjectItem(json, "cmd");
    cJSON *result = cJSON_GetObjectItem(json, "result");
    cJSON *sender_tid = cJSON_GetObjectItem(json, "sender_tid");
    cJSON *type = cJSON_GetObjectItem(json, "type");

    if (!cmd || !sender_tid || !result || !type) {
        cJSON_Delete(json);
        return -1;
    }

    msg->cmd = cmd->valueint;
    msg->result = result->valueint;
    msg->sender_tid = sender_tid->valuedouble;
    msg->type = type->valueint;

    int unpack_values = (msg->cmd == CMD_TYPE_GET && msg->result == 0);
    cJSON *key = unpack_values ? cJSON_GetObjectItem(json, "key") : NULL;
    cJSON *value1 = unpack_values ? cJSON_GetObjectItem(json, "value1") : NULL;
    cJSON *N_value2 = unpack_values ? cJSON_GetObjectItem(json, "N_value2") : NULL;
    cJSON *V_value2 = unpack_values ? cJSON_GetObjectItem(json, "V_value2") : NULL;
    cJSON *value3 = unpack_values ? cJSON_GetObjectItem(json, "value3") : NULL;
    
    if (unpack_values && (!key || !value1 || !N_value2 || !V_value2 || !value3)) {
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


int send_request(int sock, const char *json_str) {
    size_t len = strlen(json_str);
    ssize_t sent = send(sock, json_str, len, 0);
    if (sent < 0) {
        perror("send");
        return -1;
    }
    return 0;
}

int receive_response(int sock, char *buffer, size_t buffer_size) {
    memset(buffer, 0, buffer_size);
    ssize_t received = recv(sock, buffer, buffer_size - 1, 0);
    if (received <= 0) {
        perror("recv");
        return -1;
    }
    buffer[received] = '\0'; // Null-terminate the received string
    return 0;
}

int make_request(message_t *msg) {

    // get parameters for connection
    const char *server_ip = getenv("IP_TUPLAS");
    const char *server_port_str = getenv("PORT_TUPLAS");
    if (!server_ip || !server_port_str) {
        fprintf(stderr, "Error: env var not set\n");
        return -2;
    }
    int server_port = atoi(server_port_str);
    if (server_port <= 0) {
        fprintf(stderr, "Error: Invalid PORT_TUPLAS\n");
        return -2;
    }

    // to JSON
    msg->sender_tid = pthread_self();
    msg->type = MSG_TYPE_REQUEST;
    char *json_str = pack_request(msg);
    if (!json_str) {
        fprintf(stderr, "Error: Failed to convert message to JSON\n");
        return -2;
    }

    // Connecto to server
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        free(json_str);
        return -2;
    }

    struct sockaddr_in server_addr;
    struct hostent *hp;
    hp = gethostbyname(server_ip);
    if (NULL == hp) {
        perror("gethostbyname");
        return -1 ;
    }
    bzero((char *)&server_addr, sizeof(server_addr));
    memcpy (&(server_addr.sin_addr), hp->h_addr, hp->h_length);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sock);
        free(json_str);
        return -2;
    }

    // SEND
    if (send_request(sock, json_str) < 0) {
        close(sock);
        free(json_str);
        return -2;
    }
    free(json_str);

    // RECEIVE
    char buffer[BUFFER_SIZE];
    if (receive_response(sock, buffer, BUFFER_SIZE) < 0) {
        close(sock);
        return -2;
    }
    close(sock);
    // to struct
    message_t response_msg;
    if (unpack_response(buffer, &response_msg) < 0) {
        perror("Error: Invalid JSON response received\n");
        return -2;
    }

    // check if its the correct one
    if (response_msg.sender_tid != msg->sender_tid) {
        fprintf(stderr, "Got response to another request\n");
        return -2;
    }

    // overwrite on get
    msg->result = response_msg.result;
    if (msg->cmd == CMD_TYPE_GET) {
        strncpy(msg->value1, response_msg.value1, sizeof(msg->value1) - 1);
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
