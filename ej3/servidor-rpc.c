#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "claves_rpc.h"
#include "claves.h"

bool_t destroy_rpc_1_svc(int *result, struct svc_req *rqstp) {
    *result = destroy();
    return 1;
}

bool_t set_value_rpc_1_svc(set_value_peticion p, int *result, struct svc_req *rqstp) {
    
    struct Coord c = { .x = p.value3.x, .y = p.value3.y };
    *result = set_value(p.key, p.value1, p.N_value2, p.V_value2, c);
    return 1;
}


bool_t get_value_rpc_1_svc(int key, get_value_respuesta *result, struct svc_req *rqstp) {
    static char buffer[256];
    struct Coord c;
    double temp[32];
    int n = 0;

    result->status = get_value(key, buffer, &n, temp, &c);

    if (result->status != 0) {
        result->value1 = strdup("");
        result->N_value2 = 0;
        result->value3.x = 0;
        result->value3.y = 0;
        return 1;
    }


    result->value1 = strdup(buffer);
    result->N_value2 = n;
    memcpy(result->V_value2, temp, n * sizeof(double));
    result->value3.x = c.x;
    result->value3.y = c.y;


    return 1;
}



bool_t modify_value_rpc_1_svc(set_value_peticion p, int *result, struct svc_req *rqstp) {
    
    struct Coord c = { .x = p.value3.x, .y = p.value3.y };
    *result = modify_value(p.key, p.value1, p.N_value2, p.V_value2, c);
    return 1;
}

bool_t exist_rpc_1_svc(int key, int *result, struct svc_req *rqstp) {
    
    *result = exist(key);
    return 1;
}

bool_t delete_rpc_1_svc(int key, int *result, struct svc_req *rqstp) {
    
    *result = delete_key(key);
    return 1;
}


int claves_1_freeresult (SVCXPRT *transp, xdrproc_t xdr_result, caddr_t result)
{
	xdr_free (xdr_result, result);

	return 1;
}
