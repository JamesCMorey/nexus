#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include "net.h"
#include "display.h"

enum connType {
	TCP, IRC, HTTP, FTP
};

struct conn {
	int sfd;
	int type; /* connType: TCP, IRC, etc. */
};

/* global structure that encapsulates all connections */
struct netState {
	fd_set readFds;
	int max;
	struct conn conns[1024];
};

struct netState Net;

int get_conn(char *hostname, char *port)
{
	struct addrinfo hints, *res, *itr;
	int sfd, rv;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	rv = getaddrinfo(hostname, port, &hints, &res);
	if (rv) {
		display(STR, "getaddrinfo failed: %s", gai_strerror(rv));
		return -1;
	}

	for (itr = res; itr != NULL; itr = itr->ai_next) {
		sfd = socket(itr->ai_family, itr->ai_socktype,
							itr->ai_protocol);

		if (sfd == -1) {
			display(NOARG, "Socket creation failed...", NULL);
			continue;
		}


		if (connect(sfd, itr->ai_addr, itr->ai_addrlen) != -1) {
			display(NOARG, "Connection success.", NULL);
			break; // success
		}

		// failure
		close(sfd);
	}

	if (itr == NULL) {
		display(NOARG, "Failed to create connection.", NULL);
		return -1;
	}

	char info[100];
	getnameinfo(itr->ai_addr, itr->ai_addrlen, info, 100, 0, 0,
								NI_NUMERICHOST);
	display(STR, "Connected to server at %s.", info);

	freeaddrinfo(res);
	return sfd;
}

void closeAllConns() /* TODO: Implement this */
{
	/*
	for (int i = 3; i <= fds.max; i++) {
		if (FD_ISSET(i, &fds->master)) {
			display(INT, "Closing %d...", i);
			close(i);
		}
	}
	*/
}
