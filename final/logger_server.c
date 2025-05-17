#include "logger.h"
#include <stdio.h>

int *log_operation_1_svc(log_entry *entry, struct svc_req *req) {
    static int result;

    if (entry == NULL) {
        result = 1;
        return &result;
    }
    
    // print y flush del log
    printf("%s %s %s\n", entry->username, entry->operation, entry->datetime);
    fflush(stdout);

    result = 0;
    return &result;
}
