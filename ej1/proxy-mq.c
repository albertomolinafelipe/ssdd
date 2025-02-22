#include "claves.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <mqueue.h>
#include <string.h>


static int send_request(mqd_t mq, mq_message_t *msg) {
    if (mq_send(mq, (const char *)msg, sizeof(mq_message_t), 0) == -1) {
        perror("mq_send");
        return -1;
    }
    return 0;
}

static int receive_response(mqd_t mq, mq_message_t *response) {
    if (mq_receive(mq, (char *)response, sizeof(mq_message_t), NULL) == -1) {
        perror("mq_receive");
        return -1;
    }
    return 0;
}

static int make_request(mq_message_t *msg) {
    // create response mq
    char response_queue[64];
    snprintf(response_queue, sizeof(response_queue), MQ_NAME "-%d-%lu", getpid(), (unsigned long)pthread_self());
    
    // open response mq
    struct mq_attr attr = { 
        .mq_flags = 0, 
        .mq_maxmsg = 10, 
        .mq_msgsize = sizeof(mq_message_t), 
        .mq_curmsgs = 0 
    };
    mqd_t response_mq = mq_open(response_queue, O_CREAT | O_RDONLY, 0666, &attr);
    if (response_mq == (mqd_t)-1) {
        perror("mq_open (create response queue)");
        return -2;
    }

    // open MAIN mq
    mqd_t mq = mq_open(MQ_NAME, O_WRONLY);
    if (mq == (mqd_t)-1) {
        perror("mq_open (main queue)");
        mq_close(response_mq);
        mq_unlink(response_queue);
        return -2;
    }
    // populate final values in msg
    msg->sender_pid = getpid();
    msg->sender_tid = pthread_self();
    msg->type = MSG_TYPE_REQUEST;

    // Send and receive
    if (send_request(mq, msg) == -1) {
        mq_close(mq);
        mq_close(response_mq);
        mq_unlink(response_queue);
        return -2;
    }
    mq_message_t response;
    if (receive_response(response_mq, &response) == -1) {   
        mq_close(response_mq);
        mq_unlink(response_queue);
        mq_close(mq);
        return -2;
    }
    
    // only overwrite on get
    if (msg->cmd == CMD_TYPE_GET) {
        memcpy(msg, &response, sizeof(mq_message_t));
    }

    // Close and return result
    mq_close(response_mq);
    mq_unlink(response_queue);
    mq_close(mq);
    return response.result;
}



int destroy() {
    mq_message_t msg = {
        .cmd = CMD_TYPE_DESTROY
    };
    int err = make_request(&msg);
    return err;
}

int set_value(int key, char *value1, int N_value2, double *V_value2, struct Coord value3) {
    // Check format
    if (strlen(value1) > 255) return -1;
    if (N_value2 < 1 || N_value2 > 32) return -1;

    mq_message_t msg = {
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
    mq_message_t msg = {
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

    mq_message_t msg = {
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
    mq_message_t msg = {
        .cmd = CMD_TYPE_DELETE,
        .key = key,
    };
    return make_request(&msg);
}

int exist(int key) {
    mq_message_t msg = {
        .cmd = CMD_TYPE_EXIST,
        .key = key,
    };
    return make_request(&msg);
}
