#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "net.h"
#include "commands.h"
#include "display.h"

#define NUMCOMMANDS 5

static const char *COMMANDS[NUMCOMMANDS] = {	"exit", "conn", "disc", "nt",
						"clr"	};

enum command_code {
	EXIT, CONN, DISC, NEWTAB,
	CLR
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

	else if (rv == NEWTAB) {
		char *tabname = strtok(&buffer[strlen("/nt")], " ");
		mktab(tabname, -1);
	}
	else if (rv == CLR) {
		clr_cur_win();
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
	mktab(hostname, sfd);
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
