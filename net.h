#pragma once

#include <sys/select.h>

int handle_io(int sfd, fd_set master);
int get_conn(char *hostname, char *port);
