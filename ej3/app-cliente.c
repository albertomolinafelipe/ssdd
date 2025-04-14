#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <pthread.h>
#include <math.h>
#include "claves.h"

#define KEY 5
#define GREEN "\033[1;32m"
#define RED "\033[1;31m"
#define RESET "\033[0m"
int VERBOSE = 0;

void print_verbose(const char *format, ...) {
    if (!VERBOSE) return;

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

void print_test_name(const char *test_name) {
    printf(GREEN "TEST %s... " RESET, test_name);
}

int compare_values(
    const char *expected_value1,
    int expected_N_value2,
    const double *expected_V_value2,
    struct Coord expected_value3,
    const char *actual_value1,
    int actual_N_value2,
    const double *actual_V_value2,
    struct Coord actual_value3
) {
    if (strcmp(expected_value1, actual_value1) != 0) {
        printf(RED "ERROR: value1 mismatch.\nExpected: \"%s\"\nGot: \"%s\"\n" RESET,
               expected_value1, actual_value1);
        return 0;
    }

    if (expected_N_value2 != actual_N_value2) {
        printf(RED "ERROR: N_value2 mismatch.\nExpected: %d, Got: %d\n" RESET,
               expected_N_value2, actual_N_value2);
        return 0;
    }

    for (int i = 0; i < expected_N_value2; i++) {
        if (fabs(expected_V_value2[i] - actual_V_value2[i]) > 1e-6) {
            printf(RED "ERROR: V_value2[%d] mismatch.\nExpected: %f, Got: %f\n" RESET,
                   i, expected_V_value2[i], actual_V_value2[i]);
            return 0;
        }
    }

    if (expected_value3.x != actual_value3.x || expected_value3.y != actual_value3.y) {
        printf(RED "ERROR: value3 (Coord) mismatch.\nExpected: (%d, %d), Got: (%d, %d)\n" RESET,
               expected_value3.x, expected_value3.y, actual_value3.x, actual_value3.y);
        return 0;
    }

    return 1;
}

// Test 1: Set and Get a Value Successfully
void test_set_get_success() {
    print_test_name("set_get_success");

    char *value1 = "Music City USA";
    double V_value2[] = {3.14, 2.71, 1.61};
    struct Coord value3 = {100, 200};

    print_verbose("\n\tSET: \"%s\"\n", value1);
    int err = set_value(KEY, value1, 3, V_value2, value3);
    if (err == -1) {
        printf(RED "ERROR: SET failed\n" RESET);
        return;
    }

    char v1[256];
    double v2[32];
    int N_value2;
    struct Coord v3;

    err = get_value(KEY, v1, &N_value2, v2, &v3);
    if (err != 0) {
        printf(RED "ERROR: GET failed\n" RESET);
        return;
    }

    print_verbose("\tGET: \"%s\"\n", v1);
    if (!compare_values(value1, 3, V_value2, value3, v1, N_value2, v2, v3)) {
        return;
    }

    if (!VERBOSE) printf("Pass\n");
}

// Test 2: Get non-existent key
void test_get_doesnt_exist() {
    print_test_name("get_doesnt_exist");

    char v1[256];
    double v2[32];
    int N_value2;
    struct Coord v3;

    print_verbose("\n\tGET: attempt to get non-existent key\n");
    int err = get_value(KEY, v1, &N_value2, v2, &v3);
    if (err != -1) {
        printf(RED "ERROR: Expected error for getting non-existent key\n" RESET);
    } else if (!VERBOSE) {
        printf("Pass\n");
    }
}

// Test 3: Set wrong format
void test_set_wrong_format() {
    print_test_name("test_set_wrong_format");

    char *value1 = "Lonesome Drifter";
    double V_value2[] = {3.14, 2.71, 1.61};
    int N_value2 = 40;
    struct Coord value3 = {100, 200};

    print_verbose("\n\tSET: N_value %d\n", N_value2);
    int err = set_value(KEY, value1, N_value2, V_value2, value3);
    if (err != -1) {
        printf(RED "ERROR: Didn't get expected error for N_value2 > 32\n" RESET);
    }

    char long_string[300];
    memset(long_string, 'A', 299);
    long_string[299] = '\0';

    N_value2 = 3;
    print_verbose("\tSET: String length %d\n", (int)strlen(long_string));
    err = set_value(KEY, long_string, N_value2, V_value2, value3);
    if (err != -1) {
        printf(RED "ERROR: Didn't get expected error for string length > 255\n" RESET);
    } else if (!VERBOSE) printf("Pass\n");
}

// Test 4: Set already existing key
void test_set_already_existing_key() {
    print_test_name("set_already_existing_key");

    char *value1 = "Game I Can't Win";
    double V_value2[] = {3.14, 2.71, 1.61};
    struct Coord value3 = {100, 200};

    print_verbose("\n\tSET: first set\n");
    int err = set_value(KEY, value1, 3, V_value2, value3);
    if (err != 0) {
        printf(RED "ERROR: First SET failed\n" RESET);
        return;
    }

    print_verbose("\tSET: attempt to set existing key\n");
    err = set_value(KEY, value1, 3, V_value2, value3);
    if (err != -1) {
        printf(RED "ERROR: Expected error for setting an already existing key\n" RESET);
    } else if (!VERBOSE) printf("Pass\n");
}

// Test 5: Exist and delete key successfully
void test_exist_delete_success() {
    print_test_name("exist_delete_success");

    char *value1 = "Paint It Blue";
    double V_value2[] = {3.14, 2.71, 1.61};
    struct Coord value3 = {100, 200};

    print_verbose("\n\tSET: \"%s\"\n", value1);
    int err = set_value(KEY, value1, 3, V_value2, value3);
    if (err == -1) {
        printf(RED "ERROR: SET failed\n" RESET);
        return;
    }

    print_verbose("\tEXIST: before delete \n");
    err = exist(KEY);
    if (err == -1) {
        printf(RED "ERROR: Key does not exist\n" RESET);
        return;
    }

    print_verbose("\tDELETE\n");
    err = delete_key(KEY);
    if (err == -1) {
        printf(RED "ERROR: DELETE failed\n" RESET);
        return;
    }

    print_verbose("\tEXIST: after delete \n");
    err = exist(KEY);
    if (err != 0) {
        printf(RED "ERROR: Key still exists after deletion\n" RESET);
    } else if (!VERBOSE) printf("Pass\n");
}

// Test 6: Set, Modify, and Get success
void test_set_modify_get_success() {
    print_test_name("test_set_modify_get_success");

    char *value1 = "Tennesse Special";
    double V_value2[] = {1.23, 4.56, 7.89};
    struct Coord value3 = {100, 200};

    print_verbose("\n\tSET: \"%s\"\n", value1);
    int err = set_value(KEY, value1, 3, V_value2, value3);
    if (err == -1) {
        printf(RED "ERROR: SET failed\n" RESET);
        return;
    }

    char *modified_value1 = "Honest Fight";
    double modified_V_value2[] = {9.87, 6.54, 3.21};
    struct Coord modified_value3 = {300, 400};

    print_verbose("\tMODIFY: \"%s\"\n", modified_value1);
    err = modify_value(KEY, modified_value1, 3, modified_V_value2, modified_value3);
    if (err == -1) {
        printf(RED "ERROR: MODIFY failed\n" RESET);
        return;
    }

    char v1[256];
    double v2[32];
    int N_value2;
    struct Coord v3;

    err = get_value(KEY, v1, &N_value2, v2, &v3);
    if (err == -1) {
        printf(RED "ERROR: GET failed\n" RESET);
        return;
    }

    print_verbose("\tGET: \"%s\"\n", v1);
    if (!compare_values(modified_value1, 3, modified_V_value2, modified_value3, v1, N_value2, v2, v3)) {
        return;
    }

    if (!VERBOSE) {
        printf("Pass\n");
    }
}

// Test 7: Modify key that doesn't exist
void test_modify_doesnt_exist() {
    print_test_name("modify_doesnt_exist");

    char *value1 = "Bad Luck and Circumstances";
    double V_value2[] = {9.87, 6.54, 3.21};
    struct Coord value3 = {300, 400};

    print_verbose("\n\tMODIFY: attempting to modify a non-existent key\n");
    int err = modify_value(KEY, value1, 3, V_value2, value3);
    if (err != -1) {
        printf(RED "ERROR: Expected error for modifying non-existent key\n" RESET);
    } else if (!VERBOSE) {
        printf("Pass\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc > 1 && (strcmp(argv[1], "--verbose") == 0 || strcmp(argv[1], "-v") == 0)) {
        VERBOSE = 1;
    }

    destroy();
    test_set_get_success();
    destroy();
    test_set_already_existing_key();
    destroy();
    destroy();
    test_set_wrong_format();
    test_set_modify_get_success();
    destroy();
    test_exist_delete_success();
    destroy();
    test_modify_doesnt_exist();
    destroy();
    test_get_doesnt_exist();

    return 0;
}
