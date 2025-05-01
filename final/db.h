#ifndef SERVER_H
#define SERVER_H

#define USERNAME_MAX 256
#define FILE_MAX 256
#define DESCRIPTION_MAX 256
#define IP_MAX 64


/*

   User y file se almacenan como listas enlazadas
   Cada usuario tiene una lista enlazada de ficheros

   Todas las operaciones estan protegidas con mutex

*/
typedef struct file_entry {
    char filename[FILE_MAX];
    char description[DESCRIPTION_MAX];
    struct file_entry* next;
} file_entry_t;

typedef struct user_entry {
    char username[USERNAME_MAX];
    int connected;
    char ip[IP_MAX];               
    int listening_port;
    file_entry_t* published_files;
    struct user_entry* next;
} user_entry_t;


void db_init();

/**
 * @brief Libera todos los recursos de la base de datos.
 *
 * Elimina todos los usuarios registrados y sus ficheros publicados,
 * y deja la base de datos en estado inicial.
 */
void db_destroy();

/**
 * @brief Registra un nuevo usuario en la base de datos.
 *
 * @param username Nombre del usuario a registrar.
 * @return int Código de estado:
 *         - 0 si el usuario se registró correctamente.
 *         - 1 si el usuario ya existía.
 *         - 2 si ocurrió un error al reservar memoria.
 */
int db_register_user(const char* username);

/**
 * @brief Elimina un usuario existente de la base de datos.
 *
 * @param username Nombre del usuario a eliminar.
 * @return int Código de estado:
 *         - 0 si el usuario fue eliminado con éxito.
 *         - 1 si el usuario no existe.
 */
int db_unregister_user(const char* username);

/**
 * @brief Marca a un usuario como conectado y guarda su IP y puerto.
 *
 * @param username Nombre del usuario.
 * @param port Puerto en el que escucha el usuario.
 * @param ip Dirección IP del usuario.
 * @return int Código de estado:
 *         - 0 si se conectó con éxito.
 *         - 1 si el usuario no existe.
 *         - 2 si el usuario ya estaba conectado.
 */
int db_connect_user(const char* username, int port, const char* ip);

/**
 * @brief Marca a un usuario como desconectado.
 *
 * @param username Nombre del usuario.
 * @return int Código de estado:
 *         - 0 si se desconectó con éxito.
 *         - 1 si el usuario no existe.
 *         - 2 si el usuario no estaba conectado.
 */
int db_disconnect_user(const char* username);

/**
 * @brief Publica un fichero para un usuario conectado.
 *
 * @param username Nombre del usuario.
 * @param filename Nombre del fichero.
 * @param description Descripción del fichero.
 * @return int Código de estado:
 *         - 0 si el fichero se publicó con éxito.
 *         - 1 si el usuario no existe.
 *         - 2 si el usuario no está conectado.
 *         - 3 si el fichero ya fue publicado.
 *         - 4 si ocurrió un error de memoria.
 */
int db_publish(const char* username, const char* filename, const char* description);

/**
 * @brief Elimina un fichero previamente publicado por un usuario.
 *
 * @param username Nombre del usuario.
 * @param filename Nombre del fichero a eliminar.
 * @return int Código de estado:
 *         - 0 si se eliminó con éxito.
 *         - 1 si el usuario no existe.
 *         - 2 si el usuario no está conectado.
 *         - 3 si el fichero no existe.
 */
int db_delete(const char* username, const char* filename);

/**
 * @brief Lista los ficheros publicados por otro usuario.
 *
 * @param username Nombre del usuario que hace la consulta.
 * @param remote_username Nombre del usuario del que se listan los ficheros.
 * @param files_out Puntero de salida con el array de ficheros.
 * @param count_out Puntero de salida con el número de ficheros.
 * @return int Código de estado:
 *         - 0 si se obtuvo la lista con éxito.
 *         - 1 si el usuario no existe.
 *         - 2 si el usuario no está conectado.
 *         - 3 si el usuario remoto no existe.
 *         - 4 si ocurrió un error de memoria.
 */
int db_list_user_content(const char* username, const char* remote_username, file_entry_t*** files_out, int* count_out);

/**
 * @brief Lista todos los usuarios actualmente conectados.
 *
 * @param username Nombre del usuario que hace la consulta.
 * @param connected_users_out Puntero de salida con el array de usuarios conectados.
 * @param count_out Puntero de salida con el número de usuarios conectados.
 * @return int Código de estado:
 *         - 0 si se obtuvo la lista con éxito.
 *         - 1 si el usuario no existe.
 *         - 2 si el usuario no está conectado.
 *         - 3 si ocurrió un error de memoria.
 */
int db_list_connected_users(const char* username, user_entry_t*** connected_users_out, int* count_out);

#endif
