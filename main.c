#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

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

int main(int argc, char *argv[])
{
	if (argc == 3) {
		get_conn(argv[1], argv[2]);
		return -1;
	}
	else {
		puts("Usage: ./a.out [destination] [port]");
	}


	return 0;
}
