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

static mq_message_t receive_response(mqd_t mq) {
    mq_message_t response;
    if (mq_receive(mq, (char *)&response, sizeof(mq_message_t), NULL) == -1) {
        perror("mq_receive");
    }
    return response;
}

static int make_request(mq_message_t *msg) {
    char response_queue[64];
    snprintf(response_queue, sizeof(response_queue), MQ_NAME "-%d-%lu", getpid(), (unsigned long)pthread_self());

    struct mq_attr attr = { 
        .mq_flags = 0, 
        .mq_maxmsg = 10, 
        .mq_msgsize = sizeof(mq_message_t), 
        .mq_curmsgs = 0 
    };

    mqd_t response_mq = mq_open(response_queue, O_CREAT | O_RDONLY, 0666, &attr);
    if (response_mq == (mqd_t)-1) {
        perror("mq_open (create response queue)");
        return -1;
    }

    mqd_t mq = mq_open(MQ_NAME, O_WRONLY);
    if (mq == (mqd_t)-1) {
        perror("mq_open (main queue)");
        mq_close(response_mq);
        mq_unlink(response_queue);
        return -1;
    }
    
    msg->sender_pid = getpid();
    msg->sender_tid = pthread_self();
    msg->type = MSG_TYPE_REQUEST;

    if (send_request(mq, msg) == -1) {
        mq_close(mq);
        mq_close(response_mq);
        mq_unlink(response_queue);
        return -2;
    }
    
    mq_message_t response = receive_response(response_mq);
    
    if (msg->cmd == CMD_TYPE_GET) {
        memcpy(msg, &response, sizeof(mq_message_t));
    }
    
    mq_close(response_mq);
    mq_unlink(response_queue);
    mq_close(mq);
    return 0;
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
    if (N_value2 < 1 || N_value2 > 32) return -1;

    mq_message_t msg = {
        .cmd = CMD_TYPE_SET,
        .key = key,
        .N_value2 = N_value2,
        .value3 = value3,
    };
    // Limit string
    strncpy(msg.value1, value1, sizeof(msg.value1) - 1);
    msg.value1[sizeof(msg.value1) - 1] = '\0';

    memcpy(msg.V_value2, V_value2, msg.N_value2 * sizeof(double));
    int err = make_request(&msg);
    return err;
}

int get_value(int key, char *value1, int *N_value2, double *V_value2, struct Coord *value3) {
    mq_message_t msg = {
        .cmd = CMD_TYPE_GET,
        .key = key
    };

    int err = make_request(&msg);
    if (err == 0) {
        strncpy(value1, msg.value1, sizeof(msg.value1) - 1);
        *N_value2 = msg.N_value2;
        memcpy(V_value2, msg.V_value2, msg.N_value2 * sizeof(double));
        *value3 = msg.value3;
    }
    return err;
}

int modify_value(int key, char *value1, int N_value2, double *V_value2, struct Coord value3) {
    return 0;
}

int delete_key(int key) {
    return 0;
}

int exist(int key) {
    return 0;
}
