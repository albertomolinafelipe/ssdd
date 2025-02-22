#include <stdio.h>
#include <mqueue.h>
#include <unistd.h>
#include <pthread.h>
#include "claves.h"

int VERBOSE = 0;
typedef struct {
    mq_message_t msg;
} thread_arg_t;

mq_message_t get_response(mq_message_t *msg) {
   
    mq_message_t response;
    int result;

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
    return response;
}


void *handle_request(void *arg) {
    thread_arg_t *targ = (thread_arg_t *)arg;
    mq_message_t msg = targ->msg;
    free(targ);
    
    char response_queue[64];
    snprintf(response_queue, sizeof(response_queue), MQ_NAME "-%d-%lu", msg.sender_pid, (unsigned long)msg.sender_tid);
    
    struct mq_attr attr = {
        .mq_flags = 0,
        .mq_maxmsg = 10,
        .mq_msgsize = sizeof(mq_message_t),
        .mq_curmsgs = 0
    };
    
    mqd_t response_mq = mq_open(response_queue, O_CREAT | O_WRONLY, 0666, &attr);
    if (response_mq == (mqd_t)-1) {
        perror("mq_open (create response queue)");
        return NULL;
    }
    
    mq_message_t response_msg = get_response(&msg);
    if (mq_send(response_mq, (const char *)&response_msg, sizeof(response_msg), 0) == -1) {
        perror("mq_send (response)");
    }
    
    mq_close(response_mq);
    return NULL;
}

int main(int argc, char *argv[]) {    
    if (argc > 2 && (strcmp(argv[2], "--verbose") == 0 || strcmp(argv[2], "-v") == 0)) {
        VERBOSE = 1;
    }
   
    // main mq
    struct mq_attr attr = {
        .mq_flags = 0,
        .mq_maxmsg = 10,
        .mq_msgsize = sizeof(mq_message_t),
        .mq_curmsgs = 0
    };
    mqd_t mq = mq_open(MQ_NAME, O_RDONLY | O_CREAT, S_IRUSR | S_IWUSR, &attr);
    if (mq == (mqd_t)-1) {
        perror("mq_open");
        return 1;
    }
    
    while (1) {
        // read message
        mq_message_t msg;
        if (mq_receive(mq, (char *)&msg, sizeof(mq_message_t), NULL) == -1) {
            perror("mq_receive");
            break;
        }
        
        print_mq_msg(&msg, VERBOSE);
        
        // thread argument
        thread_arg_t *targ = malloc(sizeof(thread_arg_t));
        if (!targ) {
            perror("malloc");
            continue;
        }
        targ->msg = msg;
        
        // create and detach thread
        pthread_t thread;
        pthread_create(&thread, NULL, handle_request, targ);
        pthread_detach(thread);
    }
    
    mq_close(mq);
    return 0;
}

