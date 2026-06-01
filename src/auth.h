#ifndef AUTH_H
#define AUTH_H

#include "custom_types.h"

int check_pw(const char *password, Server *sv);
int load_servers(Server **out, int *count);
int save_server(const Server *sv);

#endif
