#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: %s num1 num2 ...\n", argv[0]);
        return 1;
    }
    
    for (int i = 1; i < argc; i++) {
        char *end;
        int num = strtol(argv[i], &end, 10);
        
        if (*end != '\0') {
            printf("Argumento %d = Error de conversión.\n", i);
        } else {
            printf("Argumento %d = %d\n", i, num);
        }
    }
    
    return 0;
}

