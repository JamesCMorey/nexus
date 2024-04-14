#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "net.h"
#include "commands.h"
#include "display.h"

#define NUMCOMMANDS 6

static const char *COMMANDS[NUMCOMMANDS] = {	"exit", "conn", "disc", "clr",
						"sw", "close"	};

enum command_code {
	EXIT, CONN, DISC, CLR,
	SW, CLOSE
};

static void conn_cmd(char *buffer);
static enum ConnType parse_conntype(char *buffer);

int handle_command(char *buffer)
{
	int rv;

	rv = parse_commands(buffer);

	if(rv == -1) {
		display("Command not found: %s", buffer);
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
		/* TODO make this work on values >9 */
		char *id = strtok(&buffer[strlen(":sw")], " ");
		switch_tab(id[0] - '0');
	}
	else if (rv == CLOSE) {
		int index = get_curtab_index();
		if (index == 0) {
			display("Cannot close the default tab... Did you mean \
to :exit?");
			return 0;
		}
		deltab(index);
		delconn(index);
	}

	return 0;
}

static void conn_cmd(char *buffer)
{
	char *hostname = strtok(&buffer[strlen(":conn")], " ");
	char *port  = strtok(NULL, " ");
	char *type  = strtok(NULL, " ");
	enum ConnType protocol = parse_conntype(type);

	if (port == NULL || hostname == NULL) {
		display("You need to enter the hostname and port", NULL);
		return;
	}

	int index;
	if (type == NULL) {
		index = mkconn(TCP, hostname, port);
	}
	else {
		index = mkconn(protocol, hostname, port);
	}

	mktab(hostname, index);
}

static enum ConnType parse_conntype(char *buffer)
{
	if (buffer == NULL) {
		return -1;
	}

	for (int i = 0; i < NUMCONNTYPES; i++) {
		if (!strncmp(buffer, CONNTYPES[i], strlen(CONNTYPES[i]))) {
			return i;
		}
	}
	display("protocol: %s", buffer);

	return -1;
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

void handle_normal(char *text)
{
	int rv;

	/* Need to make this cover things like FTP or http */
	if (curtab_textable()) {
		rv = send_text(text);
	}
	else {
		display("Cannot send text in this tab", NULL);
		return;
	}

	if (rv) {
		display("Failed to send.", NULL);
	}
	else {
		display(text, NULL);
	}

}
