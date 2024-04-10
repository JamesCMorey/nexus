#include <string.h>
#include <stdlib.h>
#include <ncurses.h>
#include "display.h"
#include "commands.h"
#include "log.h"


struct tab {
	char *tbname; /* Name that is displayed in the nav bar for the tab */
	WINDOW *display;
};

struct screenState {
	WINDOW *nav;
	WINDOW *display;
	WINDOW *input;

	int dy, dx;
	int ny, nx;
	int iy, ix;
	int rows, cols;
	int max_ny, max_nx;

	/* tabs organized with file descriptor such that tabs[sfd]; is the tab
	 * that is used to display communications to and from that socket. */
	struct tab tabs[1024];
	struct tab currTab; /* -1: default display 0: tabs[0] 1: tabs[1] ... */
	char buffer[1024]; /* input buffer from WINDOW *input */
};

#define CLEAR_WIN(win) werase(win); box(win, 0, 0)
#define REMOVE_LAST_CHAR(arr) arr[strlen(arr) - 1] = '\0'

/* global struct to hold all information, windows, and input from the screen */
struct screenState *Screen;

int handle_input()
{
	int rv = 0;
	char c;

	c = wgetch(Screen->input);
	strncat(Screen->buffer, &c, 1);

	/* Command handling */
	if (c == '\n' && Screen->buffer[0] == '/') {
		REMOVE_LAST_CHAR(Screen->buffer);
		rv = handle_command(Screen->buffer);

		memset(Screen->buffer, 0, sizeof(Screen->buffer));
	}
	/* Normal text handling */
	else if (c == '\n') {
		REMOVE_LAST_CHAR(Screen->buffer);
		display(STR, "%s", Screen->buffer);

		memset(Screen->buffer, 0, sizeof(Screen->buffer));
	}

	// Backspace handling
	if (c == KEY_BACKSPACE || c == KEY_DC || c == 127) {
		// need to delete DEL char and intended char
		if (strlen(Screen->buffer) > 0) {
			REMOVE_LAST_CHAR(Screen->buffer);
		}
		REMOVE_LAST_CHAR(Screen->buffer);
	}


	/* Clear the input field and redraw it, including the box around it. */
	CLEAR_WIN(Screen->input);
	mvwprintw(Screen->input, 1, 1, "%s", Screen->buffer);
	wrefresh(Screen->input); // refresh input

	return rv;
}

void display(int type, char *text, const void *arg)
{
	switch(type) {
	case NOARG:
		mvwprintw(Screen->display, ++Screen->dy, 1, "%s", text);
		break;

	case INT:
		mvwprintw(Screen->display, ++Screen->dy, 1, text,
							*(int *)arg);
		break;

	case STR:
		mvwprintw(Screen->display, ++Screen->dy, 1, text,
							(char *)arg);
		break;

	default:
		mvwprintw(Screen->display, ++Screen->dy, 1,
			"Type not supported in call to display(): %s, %d",
			__FILE__, __LINE__);
	}

	wrefresh(Screen->display);
}

void init_screen()
{
	wlog("Beginning screen initialization...\n");
	// Init functions
	initscr();
	cbreak();
	keypad(stdscr, TRUE);
	noecho();


	// initializing variables
	Screen = malloc(sizeof(struct screenState));

	//Screen->buffer = "";
	getmaxyx(stdscr, Screen->rows, Screen->cols);
	int cols = Screen->cols;
	int rows = Screen->rows;


	// windows
	Screen->nav = newwin(rows, cols/5, 0, 0);
	Screen->display = newwin(rows-2, 4*cols/5, 0, cols/5);
	Screen->input = newwin(3, 4*cols/5, rows-3, cols/5);

	refresh(); // don't know why this is necessary here


	// creating outlines for main windows
	box(Screen->nav, 0, 0);
	wrefresh(Screen->nav);

	box(Screen->display, 0, 0);
	wrefresh(Screen->display);

	box(Screen->input, 0, 0);
	wmove(Screen->input, 1, 1);
	wrefresh(Screen->input);

	getyx(Screen->display, Screen->dy, Screen->dx);
	getyx(Screen->nav, Screen->ny, Screen->nx);

	getmaxyx(Screen->nav, Screen->max_ny, Screen->max_nx);
	wlog("Completed screen initialization.\n");
}

void stop_screen()
{
	wlog("Beginning screen shutdown...\n");
	delwin(Screen->nav);
	delwin(Screen->display);
	delwin(Screen->input);
	endwin();
	wlog("Completed screen shutdown.\n");
}

