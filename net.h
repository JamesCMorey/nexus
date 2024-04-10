#pragma once

#include <sys/select.h>

enum connType {
	TCP, IRC, HTTP, FTP
};

void init_net();
void stop_net();
int get_conn(char *hostname, char *port);
void closeAllConns();
