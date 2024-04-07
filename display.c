#include <string.h>
#include <stdlib.h>
#include "net.h"
#include "display.h"
#include "commands.h"

#define CLEAR_WIN(win) werase(win); box(win, 0, 0)
#define REMOVE_LAST_CHAR(arr) arr[strlen(arr) - 1] = '\0'

struct winfo init_display()
{
	// variables
	int cols, rows;
	struct winfo wins;

	// Init functions
	initscr();
	cbreak();
	keypad(stdscr, TRUE);
	noecho();

	// initializing variables
	getmaxyx(stdscr, rows, cols);

	// windows
	wins.nav = newwin(rows, cols/5, 0, 0);
	wins.display = newwin(rows-2, 4*cols/5, 0, cols/5);
	wins.input = newwin(3, 4*cols/5, rows-3, cols/5);

	refresh(); // don't know why this is necessary here

	// creating outlines for main windows
	box(wins.nav, 0, 0);
	wrefresh(wins.nav);

	box(wins.display, 0, 0);
	wrefresh(wins.display);

	box(wins.input, 0, 0);
	wmove(wins.input, 1, 1);
	wrefresh(wins.input);

	getyx(wins.display, wins.dy, wins.dx);
	getyx(wins.nav, wins.ny, wins.nx);

	getmaxyx(wins.nav, wins.max_ny, wins.max_nx);
	return wins;
}

int handle_input(struct winfo *wins, struct fds *fds, char *buffer)
{
	int rv = 0;
	char c;

	c = wgetch(wins->input);
	strncat(buffer, &c, 1);

	// Command handling
	if (c == '\n' && buffer[0] == '/') {
		REMOVE_LAST_CHAR(buffer);
		rv = handle_command(wins, fds, buffer);

		c = ' ';
		memset(buffer, 0, sizeof(buffer));
	}

	// Backspace handling
	if (c == KEY_BACKSPACE || c == KEY_DC || c == 127) {
		// need to delete DEL char and intended char
		if (strlen(buffer) > 0) {
			REMOVE_LAST_CHAR(buffer);
		}
		REMOVE_LAST_CHAR(buffer);
	}

	// Normal text handling
	if (c == '\n') {
		REMOVE_LAST_CHAR(buffer);
		FDISPLAY("%s", buffer);

		memset(buffer, 0, sizeof(buffer));
	}

	CLEAR_WIN(wins->input);
	mvwprintw(wins->input, 1, 1, "%s", buffer);

	wrefresh(wins->input); // refresh input
	return rv;
}

int stop_display(struct winfo wins)
{
	delwin(wins.nav);
	delwin(wins.display);
	delwin(wins.input);
	endwin();
}
