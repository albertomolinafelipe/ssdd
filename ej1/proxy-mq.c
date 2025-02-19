#include "claves.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <mqueue.h>


static int send_request(mqd_t mq, mq_message_t *msg) {
    if (mq_send(mq, (const char *)msg, sizeof(mq_message_t), 0) == -1) {
        perror("mq_send");
        return -1;
    }
    return 0;
}

static void receive_response() {
    return;
}

static int make_request(cmd_type_t cmd) {
    mq_message_t msg = {
        MSG_TYPE_REQUEST, 
        cmd, 
        getpid(), 
        pthread_self()
    };
    send_request(mq, &msg);
    return 0;
}

int destroy() {
    printf("- Destroy");
    int err = make_request(CMD_TYPE_DESTROY);
    return 0;
}

int set_value(int key, char *value1, int N_value2, double *V_value2, struct Coord value3) {
    printf("- Set value");
    // Check format
    int err = make_request(CMD_TYPE_SET);
    return 0;
}

int get_value(int key, char *value1, int *N_value2, double *V_value2, struct Coord *value3) {
    printf("- Get value");
    return 0;
}

int modify_value(int key, char *value1, int N_value2, double *V_value2, struct Coord value3) {
    printf("- Modify value");
    return 0;
}

int delete_key(int key) {
    printf("- Delete key");
    return 0;
}

int exist(int key) {
    printf("- Exist");
    return 0;
}
