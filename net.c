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

#define FDISPLAY(x, y) \
mvwprintw(wins->display, ++wins->dy, 1, x, y); \
wrefresh(wins->display);

#define DISPLAY(x) \
mvwprintw(wins->display, ++wins->dy, 1, x); \
wrefresh(wins->display);

int handle_io(int sfd, fd_set master)
{
	char read[4096];
	int bytes_received;
	fd_set tmp;


	tmp = master;

	if (select(sfd + 1, &tmp, 0, 0, 0) < 0) {
		perror("select failed");
		return -1;
	}

	// get input from stdin and send it
	if (FD_ISSET(fileno(stdin), &tmp)) {
		fgets(read, sizeof(read), stdin);
		send(sfd, read, strlen(read), 0);
	}

	// get input from connection socket and print it
	if (FD_ISSET(sfd, &tmp)) {
		bytes_received = recv(sfd, read, sizeof(read), 0);

		if (bytes_received < 1) {
			printf("Connection closed by peer.\n");
			return -1;
		}
		printf("%.*s", bytes_received, read);
	}

	return 0;
}

int get_conn(struct winfo *wins, char *hostname, char *port)
{
	struct addrinfo hints, *res, *itr;
	int sfd, rv;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	rv = getaddrinfo(hostname, port, &hints, &res);
	if (rv) {
		FDISPLAY("getaddrinfo failed: %s", gai_strerror(rv));
		return -1;
	}

	for (itr = res; itr != NULL; itr = itr->ai_next) {
		sfd = socket(itr->ai_family, itr->ai_socktype,
							itr->ai_protocol);

		if (sfd == -1) {
			DISPLAY("Socket creation failed...");
			continue;
		}


		if (connect(sfd, itr->ai_addr,
						itr->ai_addrlen) != -1) {
			DISPLAY("Connection success.");

			break; // success
		}

		// failure
		close(sfd);
	}

	if (itr == NULL) {
		DISPLAY("Failed to create connection.");
		return -1;
	}

	char info[100];
	getnameinfo(itr->ai_addr, itr->ai_addrlen, info, 100, 0, 0,
								NI_NUMERICHOST);
	FDISPLAY("Connected to server at %s.", info);
	wrefresh(wins->display);

	freeaddrinfo(res);
	return sfd;
}
