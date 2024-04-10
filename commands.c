#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "net.h"
#include "commands.h"
#include "display.h"

#define NUMCOMMANDS 5

static const char *COMMANDS[NUMCOMMANDS] = {	"exit", "conn", "disc", "clr",
						"sw"	};

enum command_code {
	EXIT, CONN, DISC, CLR,
	SW
};

static void conn_cmd(char *buffer);

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
		conn_cmd(buffer);
	}

	else if (rv == CLR) {
		clr_display();
	}
	else if (rv == SW) {
		char *id = strtok(&buffer[strlen("/sw")], " ");
		switch_tab(id[0] - '0');
	}

	return 0;
}

static void conn_cmd(char *buffer)
{
	char *hostname = strtok(&buffer[strlen("/conn")], " ");
	char *port  = strtok(NULL, " ");

	if (port == NULL || hostname == NULL) {
		display(NOARG, "You need to enter the hostname and port", NULL);
		return;
	}

	int sfd = get_conn(hostname, port);

	if (sfd < 0) {
		return;
	}

	mktab(hostname, sfd);
	display(STR, "%s", "Connection successful");
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
