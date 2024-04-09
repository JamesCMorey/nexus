#include <stdlib.h>
#include <string.h>
#include "commands.h"
#include "display.h"

int handle_command(char *buffer)
{
	int rv;

	rv = parse_commands(buffer);

	if(rv == -1) {
		FDISPLAY("Command not found: %s", buffer);
	}
	else if (rv == EXIT) {
		return -1;
	}

	else if (rv == CONN) {

		char *hostname = strtok(&buffer[strlen("/conn")], " ");
		char *port  = strtok(NULL, " ");

		if (port == NULL || hostname == NULL) {
			DISPLAY("Server not found");
			return 0;
		}

		// TODO implement check for protocol
		int sfd = get_conn(wins, hostname, port);
		if (sfd < 0) {
			DISPLAY("Failed to connect.");
			return 0;
		}

		FD_SET(sfd, &fds->master);
		FD_SET(sfd, &fds->tcp);

		if (sfd > fds->max) {
			fds->max = sfd;
		}

		mvwprintw(wins->nav, 1, 1, "%s", hostname);
		wrefresh(wins->nav);
	}

	else if (rv == NEWTAB) {
		char *tabname = strtok(&buffer[strlen("/nt")], " ");

		int i;
		for (i = 0; wins->tabs[i] != NULL; i++)
			;

		wins->tabs[i] = malloc(sizeof(char) * strlen(tabname));
		strncpy(wins->tabs[i], tabname, sizeof(tabname));

		mvwprintw(wins->nav, ++wins->ny,
			(wins->max_nx - strlen(tabname))/2, "%s", wins->tabs[i]);
		wrefresh(wins->nav);
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
