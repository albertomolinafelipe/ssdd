#include <stdio.h>
#include <mqueue.h>
#include <unistd.h>
#include <pthread.h>
#include "claves.h"


int main() {

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

    mq_message_t msg;
    while (1) {
        // Receive message from the main queue
        if (mq_receive(mq, (char *)&msg, sizeof(mq_message_t), NULL) == -1) {
            perror("mq_receive");
            break;
        }
        print_mq_msg(&msg);

        // Create the response queue for each request (unique for each client)
        char response_queue[64];
        snprintf(response_queue, sizeof(response_queue), MQ_NAME "-%d-%lu", msg.sender_pid, (unsigned long)msg.sender_tid);
        

        mqd_t response_mq = mq_open(response_queue, O_CREAT | O_WRONLY, 0666, &attr);
        if (response_mq == (mqd_t)-1) {
            perror("mq_open (create response queue)");
            continue;
        }

        // Send a response to the client (this could be based on the received message)
        mq_message_t response_msg = {MSG_TYPE_RESPONSE, msg.cmd, msg.sender_pid, msg.sender_tid};
        if (mq_send(response_mq, (const char *)&response_msg, sizeof(response_msg), 0) == -1) {
            perror("mq_send (response)");
        }

        // Close the response queue after sending the response
        mq_close(response_mq);
    }

    // Close the main queue when done
    mq_close(mq);
    return 0;
}

