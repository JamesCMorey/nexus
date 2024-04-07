#pragma once

#include <ncurses.h>

struct winfo {
	WINDOW *nav;
	WINDOW *display;
	WINDOW *input;
	int dy, dx;
	int ny, nx;
	int iy, ix;
	int max_ny, max_nx;
	char *tabs[1024];
};

struct fds;

int handle_input(struct winfo *wins, struct fds *fds, char *buffer);
struct winfo init_display();
int stop_display(struct winfo wins);
