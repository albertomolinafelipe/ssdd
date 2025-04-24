#ifndef DB_H
#define DB_H

#define USERNAME_MAX 256
#define FILE_MAX 256
#define DESCRIPTION_MAX 256
#define IP_MAX 64

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
void db_destroy();

int db_register_user(const char* username);
int db_unregister_user(const char* username);

int db_connect_user(const char* username, int port, const char* ip);
int db_disconnect_user(const char* username);

int db_publish(const char* username, const char* filename, const char* description);
int db_delete(const char* username, const char* filename) ;

int db_list_user_content(const char* username, const char* remote_username, file_entry_t*** files_out, int* count_out);
int db_list_connected_users(const char* username, user_entry_t*** connected_users_out, int* count_out);

#endif
