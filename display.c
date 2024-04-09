#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include "net.h"
#include "display.h"
#include "commands.h"

#define CLEAR_WIN(win) werase(win); box(win, 0, 0)
#define REMOVE_LAST_CHAR(arr) arr[strlen(arr) - 1] = '\0'

struct tab {
	char *tbname; /* Name that is displayed in the nav bar for the tab */
	WINDOW *display;
};

struct winState {
	WINDOW *nav;
	WINDOW *display;
	WINDOW *input;

	int dy, dx;
	int ny, nx;
	int iy, ix;
	int max_ny, max_nx;

	/* tabs organized with file descriptor such that tabs[sfd]; is the tab
	 * that is used to display communications to and from that socket. */
	struct tab tabs[1024];
	struct tab currTab; /* -1: default display 0: tabs[0] 1: tabs[1] ... */
	char *buf; /* input buffer from WINDOW *input */
};

struct winState *Screen;

int handle_input()
{
	int rv = 0;
	char c;

	c = wgetch(Screen->input);
	strncat(Screen, &c, 1);

	// Command handling
	if (c == '\n' && Screen->buf[0] == '/') {
		REMOVE_LAST_CHAR(Screen->buffer);
		rv = handle_command(Screen);

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

void display(int type, char *text, void *arg)
{
	switch(type) {
		case NOARG:
			mvwprintw(wins->display, ++wins->dy, 1, text);
			break;

		case INT:
			mvwprintw(wins->display, ++wins->dy, 1, text,
								*(int *)arg);
			break;

		case STR:
			mvwprintw(wins->display, ++wins->dy, 1, text,
								(char *)arg);
			break;

		case default:
			mvwprintw(wins->display, ++wins->dy, 1,
				"Type not supported in call to display()",
				__FILE__, __LINE__);
	}

	wrefresh(wins->display);
}

void init_display()
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
	wins->nav = newwin(rows, cols/5, 0, 0);
	wins->display = newwin(rows-2, 4*cols/5, 0, cols/5);
	wins->input = newwin(3, 4*cols/5, rows-3, cols/5);

	refresh(); // don't know why this is necessary here

	// creating outlines for main windows
	box(wins->nav, 0, 0);
	wrefresh(wins->nav);

	box(wins->display, 0, 0);
	wrefresh(wins->display);

	box(wins->input, 0, 0);
	wmove(wins->input, 1, 1);
	wrefresh(wins->input);

	getyx(wins->display, wins->dy, wins->dx);
	getyx(wins->nav, wins->ny, wins->nx);

	getmaxyx(wins->nav, wins->max_ny, wins->max_nx);
}

int stop_display()
{
	delwin(wins->nav);
	delwin(wins->display);
	delwin(wins->input);
	endwin();
}
