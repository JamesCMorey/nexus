#pragma once

#include <sys/select.h>

int get_conn(char *hostname, char *port);
void closeAllConns();
