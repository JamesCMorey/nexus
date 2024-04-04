#pragma once

#include <ncurses.h>

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
};

int parse_commands(char *buffer);
int handle_input(const struct winfo wins, char *buffer);
struct winfo init_display();
int stop_display(struct winfo wins);
