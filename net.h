#pragma once

#include <sys/select.h>

struct fds {
	fd_set master;
	fd_set tcp;
};

int handle_io(int sfd, fd_set master);
int get_conn(char *hostname, char *port);
