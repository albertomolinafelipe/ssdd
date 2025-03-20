#ifndef CLAVES_H
#define CLAVES_H
#include <sys/types.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <cjson/cJSON.h>

/*
 - cmd      = 4
 - result   = 4
 - key      = 4
 - value1   <= 256 * 4
 - N_value2 = 4
 - value2   <= 32 * 4
 - value3   = 2 * 4
 - tid      = 8
 TOTAL << 2048
*/

#define BUFFER_SIZE 2048

struct Coord {
   int x ;
   int y ;
} ;

typedef enum {
    MSG_TYPE_RESPONSE,
    MSG_TYPE_REQUEST
} msg_type_t;

typedef enum {
    CMD_TYPE_DESTROY,
    CMD_TYPE_SET,
    CMD_TYPE_GET,
    CMD_TYPE_MODIFY,
    CMD_TYPE_DELETE,
    CMD_TYPE_EXIST,
    CMD_TYPE_RESPONSE,
} cmd_type_t;

// Message
typedef struct {
    cmd_type_t cmd;
    msg_type_t type;
    int result;
    unsigned long sender_tid;

    int key;
    char value1[255];
    int N_value2;
    double V_value2[32];
    struct Coord value3;
} message_t;



static inline void print_msg(message_t *msg, int detailed) {

    const char *cmd_str;
    switch (msg->cmd) {
        case CMD_TYPE_DESTROY:  cmd_str = "DESTROY"; break;
        case CMD_TYPE_SET:      cmd_str = "SET";     break;
        case CMD_TYPE_GET:      cmd_str = "GET";     break;
        case CMD_TYPE_MODIFY:   cmd_str = "MODIFY";  break;
        case CMD_TYPE_DELETE:   cmd_str = "DELETE";  break;
        case CMD_TYPE_EXIST:    cmd_str = "EXIST";   break;
        default:                cmd_str = "UNKNOWN"; break;
    }
    const char *type_str;
    switch (msg->type) {
        case MSG_TYPE_REQUEST:  type_str = "REQ"; break;
        case MSG_TYPE_RESPONSE: type_str = "RESP";   break;
        default:                type_str = "UNKNOWN"; break;
    }

    // Print the command type and sender_tid
    printf("(%s)\t%s\t%lu\n", type_str, cmd_str, msg->sender_tid);
    if (!detailed) return;

    if (msg->type == MSG_TYPE_RESPONSE) {
        printf("\tResult: %d\n", msg->result);
    }

    if ((msg->type == MSG_TYPE_REQUEST && msg->cmd != CMD_TYPE_DESTROY) || 
        (msg->type == MSG_TYPE_RESPONSE && msg->cmd == CMD_TYPE_GET && msg->result == 0) ) {
            printf("\tKey: %d\n", msg->key);
    }
    if ((msg->type == MSG_TYPE_REQUEST && (msg->cmd == CMD_TYPE_SET || msg->cmd == CMD_TYPE_MODIFY)) || 
        (msg->type == MSG_TYPE_RESPONSE && msg->cmd == CMD_TYPE_GET && msg->result == 0) ) {
            printf("\tKey: %d\n", msg->key);
            printf("\tValue1: %s\n", msg->value1);
            printf("\tN_value2: %d\n", msg->N_value2);
            printf("\tV_value2: [");
            for (int i = 0; i < msg->N_value2 && i < 32; i++) {
                printf("%lf%s", msg->V_value2[i], (i < msg->N_value2 - 1) ? ", " : "");
            }
            printf("]\n");
            printf("\tValue3: {x: %d, y: %d}\n", msg->value3.x, msg->value3.y);
    }
}

/**
 * @brief Esta llamada permite inicializar el servicio de elementos clave-valor1-valor2-valor3.
 * Mediante este servicio se destruyen todas las tuplas que estuvieran almacenadas previamente.
 * 
 * @return int La función devuelve 0 en caso de éxito y -1 en caso de error.
 * @retval 0 en caso de exito.
 * @retval -1 en caso de error.
 */
int destroy(void);

/**
 * @brief Este servicio inserta el elemento <key, value1, value2, value3>. 
 * El vector correspondiente al valor 2 vendrá dado por la dimensión del vector (N_Value2) y 
 * el vector en si (V_value2). 
 * El servicio devuelve 0 si se insertó con éxito y -1 en caso de error. 
 * Se considera error, intentar insertar una clave que ya existe previamente o 
 * que el valor N_value2 esté fuera de rango. En este caso se devolverá -1 y no se insertará. 
 * También se considerará error cualquier error en las comunicaciones.
 * 
 * 
 * @param key clave.
 * @param value1   valor1 [256].
 * @param N_value2 dimensión del vector V_value2 [1-32].
 * @param V_value2 vector de doubles [32].
 * @param value3   estructura Coord.
 * @return int El servicio devuelve 0 si se insertó con éxito y -1 en caso de error.
 * @retval 0 si se insertó con éxito.
 * @retval -1 en caso de error.
 */
int set_value(int key, char *value1, int N_value2, double *V_value2, struct Coord value3);

/**
 * @brief Este servicio permite obtener los valores asociados a la clave key. 
 * La cadena de caracteres asociada se devuelve en value1. 
 * En N_Value2 se devuelve la dimensión del vector asociado al valor 2 y en V_value2 las componentes del vector. 
 * Tanto value1 como V_value2 tienen que tener espacio reservado para poder almacenar el máximo número 
 * de elementos posibles (256 en el caso de la cadena de caracteres y 32 en el caso del vector de doubles). 
 * La función devuelve 0 en caso de éxito y -1 en caso de error, por ejemplo, 
 * si no existe un elemento con dicha clave o si se produce un error de comunicaciones.
 * 
 * 
 * @param key clave.
 * @param value1   valor1 [256].
 * @param N_value2 dimensión del vector V_value2 [1-32] por referencia.
 * @param V_value2 vector de doubles [32].
 * @param value3   estructura Coord por referencia.
 * @return int El servicio devuelve 0 si se insertó con éxito y -1 en caso de error.
 * @retval 0 en caso de éxito.
 * @retval -1 en caso de error.
 */
int get_value(int key, char *value1, int *N_value2, double *V_value2, struct Coord *value3);

/**
 * @brief Este servicio permite modificar los valores asociados a la clave key. 
 * La función devuelve 0 en caso de éxito y -1 en caso de error, por ejemplo, 
 * si no existe un elemento con dicha clave o si se produce un error en las comunicaciones. 
 * También se devolverá -1 si el valor N_value2 está fuera de rango.
 * 
 * 
 * @param key clave.
 * @param value1   valor1 [256].
 * @param N_value2 dimensión del vector V_value2 [1-32].
 * @param V_value2 vector de doubles [32].
 * @param value3   estructura Coord.
 * @return int El servicio devuelve 0 si se insertó con éxito y -1 en caso de error.
 * @retval 0 si se modificó con éxito.
 * @retval -1 en caso de error.
 */
int modify_value(int key, char *value1, int N_value2, double *V_value2, struct Coord value3);

/**
 * @brief Este servicio permite borrar el elemento cuya clave es key. 
 * La función devuelve 0 en caso de éxito y -1 en caso de error. 
 * En caso de que la clave no exista también se devuelve -1.
 * 
 * @param key clave.
 * @return int La función devuelve 0 en caso de éxito y -1 en caso de error.
 * @retval 0 en caso de éxito.
 * @retval -1 en caso de error.
 */
int delete_key(int key);

/**
 * @brief Este servicio permite determinar si existe un elemento con clave key.
 * La función devuelve 1 en caso de que exista y 0 en caso de que no exista. 
 * En caso de error se devuelve -1. Un error puede ocurrir en este caso por un problema en las comunicaciones.
 * 
 * @param key clave.
 * @return int La función devuelve 1 en caso de que exista y 0 en caso de que no exista. En caso de error se devuelve -1.
 * @retval 1 en caso de que exista.
 * @retval 0 en caso de que no exista.
 * @retval -1 en caso de error.
 */
int exist(int key);

#endif
