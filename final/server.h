#ifndef DB_H
#define DB_H


#define BACKLOG 10
#define BUF_SIZE 512

struct client_info {
    int client_fd;
    struct sockaddr_in client_addr;
};

void register_handler(int client_fd, const char* username);
void unregister_handler(int client_fd, const char* username);
void connect_handler(int client_fd, const char* buffer, const char* client_ip);
void disconnect_handler(int client_fd, const char* username);
void publish_handler(int client_fd, const char* buffer);
void delete_handler(int client_fd, const char* buffer) ;
void list_users_handler(int client_fd, const char* username) ;
void list_content_handler(int client_fd, const char* buffer) ;

#endif
