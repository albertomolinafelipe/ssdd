#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "db.h"

static user_entry_t* user_list = NULL;
static pthread_mutex_t db_mutex = PTHREAD_MUTEX_INITIALIZER;

int db_is_user_connected(const char* username) {
    pthread_mutex_lock(&db_mutex);

    user_entry_t* current = user_list;
    while (current) {
        if (strcmp(current->username, username) == 0) {
            // exito o desconectado
            int result = current->connected ? 0 : 2;
            pthread_mutex_unlock(&db_mutex);
            return result;
        }
        current = current->next;
    }

    pthread_mutex_unlock(&db_mutex);
    // user not found
    return 1;
}

void db_init() {
    user_list = NULL;
}


void db_destroy() {
    // free de todo la llist
    pthread_mutex_lock(&db_mutex);
    user_entry_t* current = user_list;
    while (current) {
        user_entry_t* tmp = current;
        current = current->next;
        free(tmp);
    }
    user_list = NULL;
    pthread_mutex_unlock(&db_mutex);
}


int db_register_user(const char* username) {
    pthread_mutex_lock(&db_mutex);

    user_entry_t* current = user_list;
    while (current) {
        if (strcmp(current->username, username) == 0) {
            pthread_mutex_unlock(&db_mutex);
            // ya existe
            return 1;
        }
        current = current->next;
    }

    user_entry_t* new_entry = malloc(sizeof(user_entry_t));
    if (!new_entry) {
        pthread_mutex_unlock(&db_mutex);
        // error no previsto
        return -1;
    }
    strncpy(new_entry->username, username, USERNAME_MAX);
    new_entry->username[USERNAME_MAX - 1] = '\0';
    new_entry->next = user_list;
    user_list = new_entry;

    pthread_mutex_unlock(&db_mutex);
    // insertado correctamente
    return 0;
}

int db_unregister_user(const char* username) {
    pthread_mutex_lock(&db_mutex);

    user_entry_t* current = user_list;
    user_entry_t* prev = NULL;

    while (current) {
        if (strcmp(current->username, username) == 0) {
            if (prev) {
                prev->next = current->next;
            } else {
                user_list = current->next;
            }
            free(current);
            pthread_mutex_unlock(&db_mutex);
            // borrado correctamente
            return 0;
        }
        prev = current;
        current = current->next;
    }

    pthread_mutex_unlock(&db_mutex);
    // no existe
    return 1;
}


int db_connect_user(const char* username, int port, const char* ip) {
    pthread_mutex_lock(&db_mutex);

    user_entry_t* current = user_list;
    while (current) {
        if (strcmp(current->username, username) == 0) {
            if (current->connected) {
                pthread_mutex_unlock(&db_mutex);
                // ya esta conectado
                return 2;
            }
            current->connected = 1;
            current->listening_port = port;
            strncpy(current->ip, ip, IP_MAX);
            current->ip[IP_MAX - 1] = '\0';
            pthread_mutex_unlock(&db_mutex);
            // conectado
            return 0;
        }
        current = current->next;
    }

    pthread_mutex_unlock(&db_mutex);
    // no existe
    return 1; 
}


int db_disconnect_user(const char* username) {
    pthread_mutex_lock(&db_mutex);

    user_entry_t* current = user_list;
    while (current) {
        if (strcmp(current->username, username) == 0) {
            if (!current->connected) {
                pthread_mutex_unlock(&db_mutex);
                // usuario no conectado
                return 2;
            }
            current->connected = 0;
            pthread_mutex_unlock(&db_mutex);
            // exito
            return 0;
        }
        current = current->next;
    }

    pthread_mutex_unlock(&db_mutex);
    // no existe
    return 1;
}


int db_publish(const char* username, const char* filename, const char* description) {
    int user_status = db_is_user_connected(username);
    // no existe
    if (user_status == 1) return 1;
    // no conectado
    if (user_status == 2) return 2;

    pthread_mutex_lock(&db_mutex);

    user_entry_t* current = user_list;
    while (current) {
        if (strcmp(current->username, username) == 0) {
            // Verificar si el fichero ya está publicado
            file_entry_t* file = current->published_files;
            while (file) {
                if (strcmp(file->filename, filename) == 0) {
                    pthread_mutex_unlock(&db_mutex);
                    return 3;
                }
                file = file->next;
            }

            // Crear nueva entrada para el fichero
            file_entry_t* new_file = malloc(sizeof(file_entry_t));
            if (!new_file) {
                pthread_mutex_unlock(&db_mutex);
                // error no previsto
                return 4;
            }
            strncpy(new_file->filename, filename, FILE_MAX);
            new_file->filename[FILE_MAX - 1] = '\0';
            strncpy(new_file->description, description, DESCRIPTION_MAX);
            new_file->description[DESCRIPTION_MAX - 1] = '\0';
            new_file->next = current->published_files;
            current->published_files = new_file;

            pthread_mutex_unlock(&db_mutex);
            // extio
            return 0;
        }
        current = current->next;
    }

    pthread_mutex_unlock(&db_mutex);
    // no existe, por si acaso
    return 1;
}


int db_delete(const char* username, const char* filename) {
    int user_status = db_is_user_connected(username);
    // no existe
    if (user_status == 1) return 1;
    // no conectado
    if (user_status == 2) return 2;

    pthread_mutex_lock(&db_mutex);

    user_entry_t* current = user_list;
    while (current) {
        if (strcmp(current->username, username) == 0) {
            file_entry_t* file = current->published_files;
            file_entry_t* prev = NULL;

            while (file) {
                if (strcmp(file->filename, filename) == 0) {
                    // Eliminar el fichero de la lista
                    if (prev) {
                        prev->next = file->next;
                    } else {
                        current->published_files = file->next;
                    }
                    free(file);
                    pthread_mutex_unlock(&db_mutex);
                    return 0;
                }
                prev = file;
                file = file->next;
            }

            pthread_mutex_unlock(&db_mutex);
            // fichero no existe
            return 3;
        }
        current = current->next;
    }

    pthread_mutex_unlock(&db_mutex);
    // por si acaso
    return 1;
}


int db_list_connected_users(const char* username, user_entry_t*** connected_users_out, int* count_out) {
 
    int user_status = db_is_user_connected(username);
    // no existe
    if (user_status == 1) return 1;
    // no conectado
    if (user_status == 2) return 2;

    pthread_mutex_lock(&db_mutex);

    // Contar usuarios conectados
    int count = 0;
    user_entry_t* current = user_list;
    while (current) {
        if (current->connected) {
            count++;
        }
        current = current->next;
    }    
    // Crear array dinámico de punteros a user_entry_t conectados
    user_entry_t** connected_users = malloc(count * sizeof(user_entry_t*));
    if (!connected_users) {
        pthread_mutex_unlock(&db_mutex);
        // error no previsto
        return 3;
    }

    int idx = 0;
    current = user_list;
    while (current) {
        if (current->connected) {
            connected_users[idx++] = current;
        }
        current = current->next;
    }

    *connected_users_out = connected_users;
    *count_out = count;

    pthread_mutex_unlock(&db_mutex);
    // exito
    return 0;
}




int db_list_user_content(const char* username, const char* remote_username, file_entry_t*** files_out, int* count_out) {
    int user_status = db_is_user_connected(username);
    // no existe
    if (user_status == 1) return 1;
    // no conectado
    if (user_status == 2) return 2;

    pthread_mutex_lock(&db_mutex);

    // Buscar al usuario remoto
    user_entry_t* remote = user_list;
    while (remote) {
        if (strcmp(remote->username, remote_username) == 0) {
            break;
        }
        remote = remote->next;
    }

    if (!remote) {
        pthread_mutex_unlock(&db_mutex);
        // no existe
        return 3;
    }

    // Contar ficheros publicados por el usuario remoto
    int count = 0;
    file_entry_t* file = remote->published_files;
    while (file) {
        count++;
        file = file->next;
    }

    // Crear array de punteros a file_entry_t
    file_entry_t** files = malloc(count * sizeof(file_entry_t*));
    if (!files) {
        pthread_mutex_unlock(&db_mutex);
        return 4;  // MEMORY ERROR
    }

    int idx = 0;
    file = remote->published_files;
    while (file) {
        files[idx++] = file;
        file = file->next;
    }

    *files_out = files;
    *count_out = count;
    pthread_mutex_unlock(&db_mutex);
    return 0;  // SUCCESS
}



