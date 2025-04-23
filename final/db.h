#ifndef DB_H
#define DB_H

void db_init();
void db_destroy();

int db_register_user(const char* username);
int db_unregister_user(const char* username);

int db_connect_user(const char* username, int port);

#endif
