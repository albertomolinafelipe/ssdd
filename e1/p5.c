#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include "lista.h"


int init(List *l) {
	*l = NULL;
	return (0);
}	

int set(List *l, char *key){
	struct Node *ptr;

	ptr = (struct Node *) malloc(sizeof(struct Node));
	if (ptr == NULL) 
		return -1;

	if (*l == NULL) {  // emtpy list
		strcpy(ptr->key, key);
		ptr->next = NULL;
		*l = ptr;
	}
	else {
		// insert in head
		strcpy(ptr->key, key);
		ptr->next = *l;
		*l = ptr;
	}


	return 0;
}	


int printList(List l){
	List aux;

	aux = l;

	while(aux != NULL){
		printf("Key=%s\n", aux->key);
		aux = aux->next;
	}
	return 0;
}	


int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: %s cadena1 cadena2 ...\n", argv[0]);
        return 1;
    }

    List  l;
    init(&l);

    for (int i = 1; i < argc; i++) {
        set(&l, argv[i]);
    }

    printList(l);

    return 0;
}
