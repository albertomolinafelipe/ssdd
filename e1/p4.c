#include <stdio.h>
#include <stdlib.h>

void ImprimirArray(char **arr, int size) {
    for (int i = 0; i < size; i++) {
        printf("Argumento %d = %s\n", i + 1, arr[i]);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: %s cadena1 cadena2 ...\n", argv[0]);
        return 1;
    }

    char **arr = malloc((argc - 1) * sizeof(char *));
    if (arr == NULL) {
        printf("Error al asignar memoria.\n");
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        arr[i - 1] = argv[i]; 
    }

    ImprimirArray(arr, argc - 1);

    free(arr);
    return 0;
}
