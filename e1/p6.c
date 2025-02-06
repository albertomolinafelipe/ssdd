#include <stdio.h>
#include <stdlib.h>

int comparar(const void *a, const void *b) {
    return (*(int *)a - *(int *)b);
}

void ObtenerMinMax(int *arr, int size, int *min, int *max) {
    *min = *max = arr[0];
    for (int i = 1; i < size; i++) {
        if (arr[i] < *min) *min = arr[i];
        if (arr[i] > *max) *max = arr[i];
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: %s num1 num2 ...\n", argv[0]);
        return 1;
    }
    
    int *arr = malloc((argc - 1) * sizeof(int));
    if (arr == NULL) {
        printf("Error al asignar memoria.\n");
        return 1;
    }
    
    for (int i = 1; i < argc; i++) {
        char *end;
        arr[i - 1] = strtol(argv[i], &end, 10);
        if (*end != '\0') {
            printf("Argumento %d = Error de conversi\xF3n.\n", i);
            free(arr);
            return 1;
        }
    }
    
    qsort(arr, argc - 1, sizeof(int), comparar);
    
    printf("Array ordenado: ");
    for (int i = 0; i < argc - 1; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");
    
    int min, max;
    ObtenerMinMax(arr, argc - 1, &min, &max);
    printf("Min: %d\n", min);
    printf("Max: %d\n", max);
    
    free(arr);
    return 0;
}
