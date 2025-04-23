#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "db.h"

#define USERNAME_MAX 256

typedef struct user_entry {
    char username[USERNAME_MAX];
    int connected;
    int listening_port;
    struct user_entry* next;
} user_entry_t;

static user_entry_t* user_list = NULL;
static pthread_mutex_t db_mutex = PTHREAD_MUTEX_INITIALIZER;

void db_init() {
    user_list = NULL;
}

void db_destroy() {
    pthread_mutex_lock(&db_mutex);
    user_entry_t* current = user_list;
    while (current) {
        user_entry_t* tmp = current;
        current = current->next;
        free(tmp);
    }
    user_list = NULL;
    pthread_mutex_unlock(&db_mutex);
}

int db_register_user(const char* username) {
    pthread_mutex_lock(&db_mutex);

    user_entry_t* current = user_list;
    while (current) {
        if (strcmp(current->username, username) == 0) {
            pthread_mutex_unlock(&db_mutex);
            return 1; // User already exists
        }
        current = current->next;
    }

    user_entry_t* new_entry = malloc(sizeof(user_entry_t));
    if (!new_entry) {
        pthread_mutex_unlock(&db_mutex);
        return -1; // Memory allocation error
    }
    strncpy(new_entry->username, username, USERNAME_MAX);
    new_entry->username[USERNAME_MAX - 1] = '\0';
    new_entry->next = user_list;
    user_list = new_entry;

    pthread_mutex_unlock(&db_mutex);
    return 0; // Success
}

int db_unregister_user(const char* username) {
    pthread_mutex_lock(&db_mutex);

    user_entry_t* current = user_list;
    user_entry_t* prev = NULL;

    while (current) {
        if (strcmp(current->username, username) == 0) {
            if (prev) {
                prev->next = current->next;
            } else {
                user_list = current->next;
            }
            free(current);
            pthread_mutex_unlock(&db_mutex);
            return 0; // Success
        }
        prev = current;
        current = current->next;
    }

    pthread_mutex_unlock(&db_mutex);
    return 1; // User not found
}

int db_connect_user(const char* username, int port) {
    pthread_mutex_lock(&db_mutex);

    user_entry_t* current = user_list;
    while (current) {
        if (strcmp(current->username, username) == 0) {
            if (current->connected) {
                pthread_mutex_unlock(&db_mutex);
                return 2;
            }
            current->connected = 1;
            current->listening_port = port;
            pthread_mutex_unlock(&db_mutex);
            return 0;
        }
        current = current->next;
    }

    pthread_mutex_unlock(&db_mutex);
    return 1;
}


