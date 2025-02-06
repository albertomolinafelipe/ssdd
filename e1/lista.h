
#ifndef _LISTA_H
#define _LISTA_H 1

#define MAX_KEY_LENGTH	256

struct Node { 
	char 	key[MAX_KEY_LENGTH];
	struct 	Node *next; 
};


typedef struct Node * List;


int init(List *l);
int set(List *l, char *key);
int printList(List l);

#endif
