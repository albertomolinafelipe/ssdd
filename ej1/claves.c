#include "claves.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Node {
    int key;
    char value1[256];
    int N_value2;
    double *V_value2;
    struct Coord value3;
    struct Node *next;
} Node;

static Node *head = NULL;

int destroy() {
    Node *current = head;
    while (current) {
        Node *next = current->next;
        free(current->V_value2);
        free(current);
        current = next;
    }
    head = NULL;
    return 0;
}

int set_value(int key, char *value1, int N_value2, double *V_value2, struct Coord value3) {
    if (N_value2 < 1 || N_value2 > 32) return -1;
    
    Node *current = head;
    while (current) {
        if (current->key == key) return -1;
        current = current->next;
    }
    
    Node *new_node = (Node *)malloc(sizeof(Node));
    if (!new_node) return -1;
    
    new_node->key = key;
    strncpy(new_node->value1, value1, 255);
    new_node->value1[255] = '\0';
    new_node->N_value2 = N_value2;
    new_node->V_value2 = (double *)malloc(N_value2 * sizeof(double));
    if (!new_node->V_value2) {
        free(new_node);
        return -1;
    }
    memcpy(new_node->V_value2, V_value2, N_value2 * sizeof(double));
    new_node->value3 = value3;
    new_node->next = head;
    head = new_node;
    return 0;
}

int get_value(int key, char *value1, int *N_value2, double *V_value2, struct Coord *value3) {
    Node *current = head;
    while (current) {
        if (current->key == key) {
            strcpy(value1, current->value1);
            *N_value2 = current->N_value2;
            memcpy(V_value2, current->V_value2, (*N_value2) * sizeof(double));
            *value3 = current->value3;
            return 0;
        }
        current = current->next;
    }
    return -1;
}

int modify_value(int key, char *value1, int N_value2, double *V_value2, struct Coord value3) {
    if (N_value2 < 1 || N_value2 > 32) return -1;
    
    Node *current = head;
    while (current) {
        if (current->key == key) {
            strncpy(current->value1, value1, 255);
            current->value1[255] = '\0';
            current->N_value2 = N_value2;
            free(current->V_value2);
            current->V_value2 = (double *)malloc(N_value2 * sizeof(double));
            if (!current->V_value2) return -1;
            memcpy(current->V_value2, V_value2, N_value2 * sizeof(double));
            current->value3 = value3;
            return 0;
        }
        current = current->next;
    }
    return -1;
}

int delete_key(int key) {
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
            return 0;
        }
        prev = current;
        current = current->next;
    }
    return -1;
}

int exist(int key) {
    Node *current = head;
    while (current) {
        if (current->key == key) {
            return 1;
        }
        current = current->next;
    }
    return 0;
}
