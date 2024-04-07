#pragma once

#include "net.h"

#define NUMCOMMANDS 4

static const char *COMMANDS[NUMCOMMANDS] = {"exit", "conn", "disc", "nt"};

#define FDISPLAY(x, y) \
mvwprintw(wins->display, ++wins->dy, 1, x, y); \
wrefresh(wins->display);

#define DISPLAY(x) \
mvwprintw(wins->display, ++wins->dy, 1, x); \
wrefresh(wins->display);

enum command_code {
	EXIT,
	CONN,
	DISC,
	NEWTAB
};

int handle_command(struct winfo *wins, struct fds *fds, char *buffer);
int parse_commands(char *buffer);
