#pragma once

#include <ncurses.h>

#define NUMCOMMANDS 3

static const char *COMMANDS[NUMCOMMANDS] = {"exit", "conn", "disc"};

#define FDISPLAY(x, y) \
mvwprintw(wins->display, ++wins->dy, 1, x, y); \
wrefresh(wins->display);

#define DISPLAY(x) \
mvwprintw(wins->display, ++wins->dy, 1, x); \
wrefresh(wins->display);

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

struct fds;

int handle_command(struct winfo *wins, struct fds *fds, char *buffer);
int parse_commands(char *buffer);
int handle_input(struct winfo *wins, struct fds *fds, char *buffer);
struct winfo init_display();
int stop_display(struct winfo wins);
