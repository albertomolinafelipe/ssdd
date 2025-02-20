#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "claves.h"


int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s --set | --get\n", argv[0]);
        return 1;
    }

    int key = 5;

    if (strcmp(argv[1], "--set") == 0) {
        char *value1 = "ejemplo de valor 1";
        double V_value2[] = {2.3, 0.5, 23.45};
        struct Coord value3 = {10, 5};

        int err = set_value(key, value1, 3, V_value2, value3);
        if (err == -1) {
            printf("SET error\n");
            return 1;
        }
        printf("SET success\n");

    } else if (strcmp(argv[1], "--get") == 0) {
        char v1[256];
        double v2[32];
        int N_value2;
        struct Coord v3;

        int err = get_value(key, v1, &N_value2, v2, &v3);
        if (err == -1) {
            printf("GET error\n");
            return 1;
        }
        printf("GET result: v1=%s, N_value2=%d, v3=(%d, %d)\n", v1, N_value2, v3.x, v3.y);

    } else {
        printf("Invalid argument. Use --set or --get\n");
        return 1;
    }

    return 0;
}
