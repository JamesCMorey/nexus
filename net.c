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

	if (c->sfd > Net->maxsfd) {
		Net->maxsfd = c->sfd;
	}

	FD_SET(c->sfd, &Net->readfds);
	Net->conns[Net->maxindex + 1] = c;
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

static struct conn *ind_get_conn(int tab_index)
{
	/* - 1 to convert from tabindex to connindex tabs[1] -> conns[0] */
	if (Net->conns[tab_index - 1] == NULL) {
		return NULL;
	}

	return Net->conns[tab_index - 1];
}

/* TODO implement this so when a sfd is ready to be read, the connection and
 * thereby the index of the tab can be found */
struct conn *sfd_get_conn(int sfd)
{
	return NULL;
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

int send_text(char *text)
{
	int rv;
	char msg[strlen(text) + 2];
	strncpy(msg, text, strlen(text));
	msg[strlen(text)] = '\n';
	msg[strlen(text) + 1] = '\0';

	rv = send(Net->curconn->sfd, msg, strlen(msg), 0);
	if (rv == strlen(msg)) {
		display("Sent %d bytes to %s", rv, Net->curconn->hostname);
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
	Net->maxindex = 0;
	Net->conn_count = 0;

	wlog("Network initialization completed.");
}

void delconn(int index)
{
	close(Net->conns[index]->sfd);
	free(Net->conns[index]);

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
		display("getaddrinfo failed: %s", gai_strerror(rv));
		return -1;
	}

	for (itr = res; itr != NULL; itr = itr->ai_next) {
		sfd = socket(itr->ai_family, itr->ai_socktype,
							itr->ai_protocol);

		if (sfd == -1) {
			display("Socket creation failed...");
			continue;
		}


		if (connect(sfd, itr->ai_addr, itr->ai_addrlen) != -1) {
			display("Connection success.");
			break; // success
		}

		// failure
		close(sfd);
	}

	if (itr == NULL) {
		display("Failed to create connection.");
		return -1;
	}

	char info[100];
	getnameinfo(itr->ai_addr, itr->ai_addrlen, info, 100, 0, 0,
								NI_NUMERICHOST);
	display("Connected to server at %s.", info);

	freeaddrinfo(res);
	return sfd;
}
