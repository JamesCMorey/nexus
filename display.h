#pragma once

#include <ncurses.h>
#include "net.h"

#define NUMCOMMANDS 3

static const char *COMMANDS[NUMCOMMANDS] = {"exit", "conn", "disc"};

enum command_code {
	EXIT,
	CONN,
	DISC
};

struct winfo {
	WINDOW *nav;
	WINDOW *display;
	WINDOW *input;
	int dy, dx;
	int ny, nx;
	int iy, ix;
};

int handle_command(struct winfo wins, struct fds *fds, char *buffer);
int parse_commands(char *buffer);
int handle_input(const struct winfo wins, struct fds *fds, char *buffer);
struct winfo init_display();
int stop_display(struct winfo wins);
