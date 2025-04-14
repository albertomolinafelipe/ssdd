#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rpc/rpc.h>
#include "claves_rpc.h"
#include "claves.h"

static CLIENT *get_rpc_client() {
    const char *server_ip = getenv("IP_TUPLAS");
    if (!server_ip) {
        fprintf(stderr, "[ERROR] Variable IP_TUPLAS no definida\n");
        return NULL;
    }

    CLIENT *clnt = clnt_create(server_ip, CLAVES, CLAVESVER, "tcp");
    if (!clnt) {
        clnt_pcreateerror("[ERROR] No se pudo crear el cliente RPC");
    }

    return clnt;
}

int destroy() {
    CLIENT *clnt = get_rpc_client();
    if (!clnt) return -2;

    int resultado;
    enum clnt_stat status = destroy_rpc_1(&resultado, clnt);
    clnt_destroy(clnt);

    return (status == RPC_SUCCESS) ? resultado : -2;
}

int set_value(int key, char *value1, int N_value2, double *V_value2, struct Coord value3) {
    if (strlen(value1) > 255 || N_value2 < 1 || N_value2 > 32) return -1;

    set_value_peticion p;
    p.key = key;
    p.value1 = value1;
    p.N_value2 = N_value2;
    memcpy(p.V_value2, V_value2, N_value2 * sizeof(double));
    p.value3.x = value3.x;
    p.value3.y = value3.y;

    CLIENT *clnt = get_rpc_client();
    if (!clnt) return -2;

    int resultado;
    enum clnt_stat status = set_value_rpc_1(p, &resultado, clnt);
    clnt_destroy(clnt);

    return (status == RPC_SUCCESS) ? resultado : -2;
}

int get_value(int key, char *value1, int *N_value2, double *V_value2, struct Coord *value3) {
    if (!value1 || !N_value2 || !V_value2 || !value3) return -2;


    CLIENT *clnt = get_rpc_client();
    if (!clnt) return -2;

    get_value_respuesta respuesta;
    memset(&respuesta, 0, sizeof(get_value_respuesta));

    enum clnt_stat status = get_value_rpc_1(key, &respuesta, clnt);
    clnt_destroy(clnt);
    
    if (status != RPC_SUCCESS) return -2;
    if (respuesta.status != 0) return -1;

    if (!respuesta.value1 || !respuesta.V_value2 || respuesta.N_value2 < 0 || respuesta.N_value2 > 32)
        return -1;

    strncpy(value1, respuesta.value1, 255);
    value1[255] = '\0';
    *N_value2 = respuesta.N_value2;
    memcpy(V_value2, respuesta.V_value2, respuesta.N_value2 * sizeof(double));
    value3->x = respuesta.value3.x;
    value3->y = respuesta.value3.y;

    return 0;
}

int modify_value(int key, char *value1, int N_value2, double *V_value2, struct Coord value3) {

    if (strlen(value1) > 255 || N_value2 < 1 || N_value2 > 32) return -1;

    set_value_peticion p;
    p.key = key;
    p.value1 = value1;
    p.N_value2 = N_value2;
    memcpy(p.V_value2, V_value2, N_value2 * sizeof(double));
    p.value3.x = value3.x;
    p.value3.y = value3.y;

    CLIENT *clnt = get_rpc_client();
    if (!clnt) return -2;

    int resultado;
    enum clnt_stat status = modify_value_rpc_1(p, &resultado, clnt);
    clnt_destroy(clnt);

    return (status == RPC_SUCCESS) ? resultado : -2;
}

int delete_key(int key) {

    CLIENT *clnt = get_rpc_client();
    if (!clnt) return -2;

    int resultado;
    enum clnt_stat status = delete_rpc_1(key, &resultado, clnt);
    clnt_destroy(clnt);

    return (status == RPC_SUCCESS) ? resultado : -2;
}

int exist(int key) {

    CLIENT *clnt = get_rpc_client();
    if (!clnt) return -2;

    int resultado;
    enum clnt_stat status = exist_rpc_1(key, &resultado, clnt);
    clnt_destroy(clnt);

    return (status == RPC_SUCCESS) ? resultado : -2;
}
