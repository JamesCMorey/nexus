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
	int maxindex;
	int conn_count;
	struct conn *conns[1024]; /* TODO implement position reuse */
	struct conn *curconn;
};

static struct conn *ind_get_conn(int index);

struct netState *Net;

const char *CONNTYPES[NUMCONNTYPES] = { "tcp", "irc", "http", "ftp" };

int mkconn(enum ConnType type, char *hostname, char *port)
{
	struct conn *c = malloc(sizeof(struct conn));

	c->hostname = malloc(strlen(hostname) + 1);
	strncpy(c->hostname, hostname, strlen(hostname) + 1);
	c->sfd = mksfd(hostname, port);
	c->type = type;
	/* This intentionally skips of index 0 in the array (that index is taken
	 * up by the default tab in the tabs array)*/
	c->index = Net->maxindex + 1;
	Net->conns[Net->maxindex + 1] = c;

	if (c->sfd > Net->maxsfd) {
		Net->maxsfd = c->sfd;
	}

	FD_SET(c->sfd, &Net->readfds);
	Net->curconn = c;
	Net->maxindex++;
	Net->conn_count++;
	return c->index;
}

int get_conntype(int index)
{
	struct conn *c = ind_get_conn(index);
	if (c == NULL) {
		return -1;
	}

	return c->type;
}

static struct conn *ind_get_conn(int index)
{
	/* - 1 to convert from tabindex to connindex tabs[1] -> conns[0] */
	if (Net->conns[index] == NULL) {
		return NULL;
	}

	return Net->conns[index];
}

/* TODO implement this so when a sfd is ready to be read, the connection and
 * thereby the index of the tab can be found */
int sfd_get_conn_index(int sfd)
{
	for (int i = 1; i <= Net->maxsfd; i++) {
		if (Net->conns[i] != NULL && Net->conns[i]->sfd == sfd) {
			return Net->conns[i]->index;
		}
	}

	return -1;
}

void switch_conn(int index)
{
	struct conn *c = ind_get_conn(index);
	if (c == NULL) {
		display("Could not switch conn");
		return;
	}

	Net->curconn = c;
}

/* TODO restrict message size */
int send_text(char *text)
{
	int rv;
	char msg[strlen(text) + 3];

	if (Net->curconn == NULL) {
		display("No connection specified");
		return -1;
	}

	strncpy(msg, text, strlen(text));
	msg[strlen(text)] = '\r';
	msg[strlen(text) + 1] = '\n';
	msg[strlen(text) + 2] = '\0';

	rv = send(Net->curconn->sfd, msg, strlen(msg), 0);
	if (rv == strlen(msg)) {
		add_to_default("Sent %d bytes to %s", rv, Net->curconn->hostname);
		return 0;
	}

	return -1;
}

int read_conn(int index)
{
	int rv;
	enum ConnType type = Net->conns[index]->type;

	switch(type) {
	case TCP:
		rv = recv_text();
		break;
	default:
		display("Conn type not found.");
		rv = -1;
	}

	return rv;
}

int recv_text(int index)
{
	int rv;
	char buf[500];
	rv = recv(Net->conns[index]->sfd, buf, sizeof(buf), 0);

	if (rv == 0) {
		add_to_tab(index, "Connection closed by peer.");
		delconn(Net->curconn->index);
		return -1;
	}

	if (rv < 0) {
		display("An error has occurred during message reception.");
		return -1;
	}

	size_add_to_tab(index, buf, rv);
	return 0;
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
	Net->maxindex = 0;
	Net->conn_count = 0;

	wlog("Network initialization completed.");
}

void delconn(int index)
{
	if (Net->conns[index] == NULL) {
		return;
	}

	for (int i = index - 1; i >= 0; i--) {
		if (Net->conns[index]->sfd == Net->maxsfd &&
							Net->conns[i] != NULL) {
			Net->maxsfd = Net->conns[i]->sfd;
		}

		if (Net->conns[i] != NULL) {
			Net->curconn = Net->conns[i];
			break;
		}
	}

	FD_CLR(Net->conns[index]->sfd, &Net->readfds);
	close(Net->conns[index]->sfd);
	free(Net->conns[index]);
	Net->conns[index] = NULL;
}

int get_maxsfd(void)
{
	return Net->maxsfd;
}

void stop_net()
{
	wlog("Shutting down network...");

	for (int i = 0; i <= Net->maxindex; i++) {
		if(ind_get_conn(i) != NULL) {
			delconn(i);
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
		add_to_default("getaddrinfo failed: %s", gai_strerror(rv));
		return -1;
	}

	for (itr = res; itr != NULL; itr = itr->ai_next) {
		sfd = socket(itr->ai_family, itr->ai_socktype,
							itr->ai_protocol);

		if (sfd == -1) {
			add_to_default("Socket creation failed...");
			continue;
		}


		if (connect(sfd, itr->ai_addr, itr->ai_addrlen) != -1) {
			add_to_default("Connection success.");
			break; // success
		}

		// failure
		close(sfd);
	}

	if (itr == NULL) {
		add_to_default("Failed to create connection.");
		return -1;
	}

	char info[100];
	getnameinfo(itr->ai_addr, itr->ai_addrlen, info, 100, 0, 0,
								NI_NUMERICHOST);
	add_to_default("Connected to server at %s.", info);

	freeaddrinfo(res);
	return sfd;
}
