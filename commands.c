#include <stdlib.h>
#include <string.h>
#include "net.h"
#include "commands.h"
#include "display.h"

#define NUMCOMMANDS 4

static const char *COMMANDS[NUMCOMMANDS] = {"exit", "conn", "disc", "nt"};

enum command_code {
	EXIT,
	CONN,
	DISC,
	NEWTAB
};

int handle_command(char *buffer)
{
	int rv;

	rv = parse_commands(buffer);

	if(rv == -1) {
		display(STR, "Command not found: %s", buffer);
	}
	else if (rv == EXIT) {
		return -1;
	}

	else if (rv == CONN) {

		char *hostname = strtok(&buffer[strlen("/conn")], " ");
		char *port  = strtok(NULL, " ");

		if (port == NULL || hostname == NULL) {
			display(NOARG, "You need to enter the hostname and port",
				NULL);
			return 0;
		}

		// TODO implement check for protocol
		int sfd = get_conn(hostname, port);
		if (sfd < 0) {
			display(NOARG, "Failed to connect.", NULL);
			return 0;
		}

		/* mktab(hostname); TODO: implement mktab */
	}

	else if (rv == NEWTAB) {
		//char *tabname = strtok(&buffer[strlen("/nt")], " ");
	}

	return 0;
}

int parse_commands(char *buffer)
{
	for (int i = 0; i < NUMCOMMANDS; i++) {
		if (!strncmp(&buffer[1], COMMANDS[i], strlen(COMMANDS[i]))) {
			return i;
		}
	}

	return -1;
}
