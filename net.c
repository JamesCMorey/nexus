#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include "net.h"
#include "log.h"
#include "display.h"

struct conn {
	char *hostname;
	int sfd;
	int index;
	enum ConnType type; /* connType: TCP, IRC, etc. */
};

/* global structure that encapsulates all connections */
struct netState {
	fd_set readfds;
	fd_set writefds;
	int maxsfd;
	int next_open_index;
	int conn_count;
	struct conn *conns[1024]; /* TODO implement position reuse */
	struct conn *curconn;
};

static struct conn *get_conn(int index);

struct netState *Net;

int mkconn(enum ConnType type, char *hostname, char *port)
{
	struct conn *c = malloc(sizeof(struct conn));

	Net->conns[Net->next_open_index] = c;
	c->sfd = mksfd(hostname, port);
	c->type = type;
	c->index = Net->next_open_index;

	if (c->sfd > Net->maxsfd) {
		Net->maxsfd = c->sfd;
	}

	FD_SET(c->sfd, &Net->readfds);
	Net->curconn = c;
	Net->next_open_index++;
	Net->conn_count++;
	return c->index;
}

int get_conntype(int index)
{
	struct conn *c = get_conn(index);
	if (c == NULL) {
		return -1;
	}

	return c->type;
}

static struct conn *get_conn(int index)
{
	/* - 1 to convert from tabindex to connindex */
	if (Net->conns[index - 1] == NULL) {
		return NULL;
	}

	return Net->conns[index - 1];
}

void switch_conn(int sfd)
{
	for (int i = 0; i < Net->next_open_index; i++) {
		if(Net->conns[i]->sfd == sfd) {
			Net->curconn = Net->conns[i];
			return;
		}
	}
	display(INT, "sfd (%d) could not be found in netState.", &sfd);
}

int send_text(char *text)
{
	int rv;

	rv = send(Net->curconn->sfd, text, strlen(text), 0);
	if (rv == strlen(text)) {
		display(INT, "Sent %d bytes to ---", &rv);
		return 0;
	}

	return -1;
}

fd_set get_readfds(void)
{
	return Net->readfds;
}

fd_set get_writefds(void)
{
	return Net->writefds;
}

void init_net()
{
	wlog("Initializing network...");
	Net = malloc(sizeof(struct netState));

	FD_ZERO(&Net->readfds);
	FD_SET(fileno(stdin), &Net->readfds); /* Don't ignore user input */
	Net->maxsfd = fileno(stdin);
	/* Setting next_open_index to 1 makes up for the default first tab in
	 * Screen->tabs[] in display.c. */
	Net->next_open_index = 1;
	Net->conn_count = 0;

	wlog("Network initialization completed.");
}

void stop_net()
{
	wlog("Shutting down network...");

	for (int i = 0; i <= Net->maxsfd; i++) {
		if(Net->conns[i] != NULL) {
			close(Net->conns[i]->sfd);
			free(Net->conns[i]);
		}
	}

	free(Net);
	wlog("Network shutdown complete.");
}

int mksfd(char *hostname, char *port)
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
