#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>

int handle_io(int sfd, fd_set master);
int get_conn(char *hostname, char *port);

int main(int argc, char *argv[])
{
	int sfd;
	fd_set master;

	if (argc == 3) {
		sfd = get_conn(argv[1], argv[2]);
	}
	else {
		puts("Usage: ./a.out [destination] [port]");
		return -1;
	}

	FD_ZERO(&master);
	FD_SET(fileno(stdin), &master);
	FD_SET(sfd, &master);

	while (1) {
		if (handle_io(sfd, master) < 0) {
			break;
		}
	}

	close(sfd);
	return 0;
}

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

int get_conn(char *hostname, char *port)
{
	struct addrinfo hints, *res, *itr;
	int sfd, rv;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	rv = getaddrinfo(hostname, port, &hints, &res);
	if (rv) {
		fprintf(stderr, "getaddrinfo failed: %s\n", gai_strerror(rv));
		return -1;
	}

	for (itr = res; itr != NULL; itr = itr->ai_next) {
		sfd = socket(itr->ai_family, itr->ai_socktype,
							itr->ai_protocol);

		if (sfd == -1) {
			fprintf(stderr, "Socket creation failed...\n");
			continue;
		}

		if (connect(sfd, itr->ai_addr,
						itr->ai_addrlen) != -1) {
			puts("Connection success.");
			break; // success
		}

		// failure
		close(sfd);
	}

	if (itr == NULL) {
		fprintf(stderr, "Failed to create connection.\n");
		return -1;
	}

	char info[100];
	getnameinfo(itr->ai_addr, itr->ai_addrlen, info, 100, 0, 0,
								NI_NUMERICHOST);
	printf("Connected to server at %s.\n", info);

	freeaddrinfo(res);
	return sfd;
}
