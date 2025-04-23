#ifndef DB_H
#define DB_H

#define USERNAME_MAX 256

typedef struct user_entry {
    char username[USERNAME_MAX];
    int connected;
    int listening_port;
    struct user_entry* next;
} user_entry_t;

void db_init();
void db_destroy();

int db_register_user(const char* username);
int db_unregister_user(const char* username);

int db_connect_user(const char* username, int port);
int db_disconnect_user(const char* username);

int db_list_connected_users(const char* username, user_entry_t*** connected_users_out, int* count_out);

#endif
