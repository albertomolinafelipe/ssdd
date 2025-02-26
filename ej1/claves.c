#include "claves.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

typedef struct Node {
    int key;
    char value1[256];
    int N_value2;
    double *V_value2;
    struct Coord value3;
    struct Node *next;
} Node;

static Node *head = NULL;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int destroy() {
    pthread_mutex_lock(&mutex);
    // free every node
    Node *current = head;
    while (current) {
        Node *next = current->next;
        free(current->V_value2);
        free(current);
        current = next;
    }
    head = NULL;
    pthread_mutex_unlock(&mutex);
    return 0;
}

int set_value(int key, char *value1, int N_value2, double *V_value2, struct Coord value3) {
    // check format
    if (N_value2 < 1 || N_value2 > 32) return -1;
    
    pthread_mutex_lock(&mutex);
    // go to last element, check if key already exists
    Node *current = head;
    while (current) {
        if (current->key == key) {
            pthread_mutex_unlock(&mutex);
            return -1;
        }
        current = current->next;
    }
    
    // new node
    Node *new_node = (Node *)malloc(sizeof(Node));
    if (!new_node) {
        pthread_mutex_unlock(&mutex);
        return -1;
    }
    
    // populate new node
    // only copy 255 characters, even if its shorter
    new_node->key = key;
    strncpy(new_node->value1, value1, 255);
    new_node->value1[255] = '\0';
    new_node->N_value2 = N_value2;
    new_node->V_value2 = (double *)malloc(N_value2 * sizeof(double));
    if (!new_node->V_value2) {
        free(new_node);
        pthread_mutex_unlock(&mutex);
        return -1;
    }
    memcpy(new_node->V_value2, V_value2, N_value2 * sizeof(double));
    new_node->value3 = value3;
    new_node->next = head;
    head = new_node;
    
    pthread_mutex_unlock(&mutex);
    return 0;
}

int get_value(int key, char *value1, int *N_value2, double *V_value2, struct Coord *value3) {
    pthread_mutex_lock(&mutex);
    // find key, copy value and return
    Node *current = head;
    while (current) {
        if (current->key == key) {
            // value1 should be at least 255
            strcpy(value1, current->value1);
            *N_value2 = current->N_value2;
            // value2 should be at least 32
            memcpy(V_value2, current->V_value2, (*N_value2) * sizeof(double));
            *value3 = current->value3;
            pthread_mutex_unlock(&mutex);
            return 0;
        }
        current = current->next;
    }
    pthread_mutex_unlock(&mutex);
    // key not found
    return -1;
}

int modify_value(int key, char *value1, int N_value2, double *V_value2, struct Coord value3) {
    // check format
    if (N_value2 < 1 || N_value2 > 32) return -1;
    
    pthread_mutex_lock(&mutex);
    Node *current = head;
    // find key and set values
    while (current) {
        if (current->key == key) {
            strncpy(current->value1, value1, 255);
            current->value1[255] = '\0';
            current->N_value2 = N_value2;
            free(current->V_value2);
            current->V_value2 = (double *)malloc(N_value2 * sizeof(double));
            if (!current->V_value2) {
                pthread_mutex_unlock(&mutex);
                return -1;
            }
            memcpy(current->V_value2, V_value2, N_value2 * sizeof(double));
            current->value3 = value3;
            pthread_mutex_unlock(&mutex);
            return 0;
        }
        current = current->next;
    }
    pthread_mutex_unlock(&mutex);
    // key not found
    return -1;
}

int delete_key(int key) {
    pthread_mutex_lock(&mutex);
    // find and free key, fix linked list
    Node *current = head, *prev = NULL;
    while (current) {
        if (current->key == key) {
            if (prev) {
                prev->next = current->next;
            } else {
                head = current->next;
            }
            free(current->V_value2);
            free(current);
            pthread_mutex_unlock(&mutex);
            return 0;
        }
        prev = current;
        current = current->next;
    }
    pthread_mutex_unlock(&mutex);
    // key not found
    return -1;
}

int exist(int key) {
    pthread_mutex_lock(&mutex);
    // look for key
    Node *current = head;
    while (current) {
        if (current->key == key) {
            pthread_mutex_unlock(&mutex);
            return 1;
        }
        current = current->next;
    }
    pthread_mutex_unlock(&mutex);
    // key not found
    return 0;
}
