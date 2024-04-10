#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include "display.h"
#include "commands.h"
#include "log.h"

#define CLEAR_WIN(win) werase(win); box(win, 0, 0)
#define REMOVE_LAST_CHAR(arr) arr[strlen(arr) - 1] = '\0'


struct tab {
	char tabname[10]; /* Name that is displayed in the nav bar for the tab */
	int unread:1; /* Flag for unseen activity in tab */
	int sfd; /* -1 means local/not networked */
	WINDOW *win;
	int cur_y, cur_x;
};

struct screenState {
	int rows, cols;

	WINDOW *nav;
	int ny, nx;
	int max_ny, max_nx;

	WINDOW *display;
	int dy, dx;

	WINDOW *input;
	int iy, ix;
	char buffer[1024]; /* input buffer from WINDOW *input */

	/* tabs organized with file descriptor such that tabs[sfd]; is the tab
	 * that is used to display communications to and from that socket. */
	struct tab *tabs[1025];
	struct tab *curtab; /* -1: default display 0: tabs[0] 1: tabs[1] ... */
	int maxtab;
};

/* global struct to hold all information, windows, and input from the screen */
struct screenState *Screen;

static void clrwin(WINDOW *win);
static void deltab(struct tab *tb);
static void show_tabs();

/* INPUT */
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

/* MISC */
void display(int type, char *text, const void *arg)
{
	switch(type) {
	case NOARG:
		mvwprintw(Screen->curtab->win, ++Screen->dy, 1, "%s", text);
		break;

	case INT:
		mvwprintw(Screen->curtab->win, ++Screen->dy, 1, text,
							*(int *)arg);
		break;

	case STR:
		mvwprintw(Screen->curtab->win, ++Screen->dy, 1, text,
							(char *)arg);
		break;

	default:
		mvwprintw(Screen->curtab->win, ++Screen->dy, 1,
			"Type not supported in call to display(): %s, %d",
			__FILE__, __LINE__);
	}

	wrefresh(Screen->curtab->win);
}

void clr_cur_win()
{
	clrwin(Screen->curtab->win);
}

static void clrwin(WINDOW *win)
{
	werase(win);
	box(win, 0, 0);
	wrefresh(win);
}

/* ===== TABBING ====== */
void mktab(char *name, int sfd)
{
	int i = sfd;
	while (i < 0 || Screen->tabs[i] != NULL) {
		i++;
	}
	Screen->tabs[i] = malloc(sizeof(struct tab));
	strncpy(Screen->tabs[i]->tabname, name, sizeof(char) * 10);
	Screen->tabs[i]->win = newwin(Screen->rows-2, 4*Screen->rows-2, 0, Screen->cols/5);
	Screen->tabs[i]->sfd = sfd; /* Allows matching network conn to tab */

	if (i > Screen->maxtab) {
		Screen->maxtab = i;
	}
	show_tabs();
}

static void deltab(struct tab *tb)
{
	delwin(tb->win);
	free(tb);
}

static void show_tabs()
{
	clrwin(Screen->nav);
	Screen->ny = 0;

	for (int i = 0; i <= Screen->maxtab; i++) {
		if (Screen->tabs[i] != NULL) { // monstrosity of line breaks
			char *name = Screen->tabs[i]->tabname;

			mvwprintw(Screen->nav, ++Screen->ny,
				(Screen->max_nx-strlen(name))/2, "%s", name);
		}
	}
	wrefresh(Screen->nav);
}

/* ====== INIT AND CLEANUP ====== */
void init_screen()
{
	wlog("Initializing screen...");
	// Init functions
	initscr();
	cbreak();
	keypad(stdscr, TRUE);
	noecho();

	// initializing variables
	Screen = malloc(sizeof(struct screenState));
	Screen->maxtab = 0;
	getmaxyx(stdscr, Screen->rows, Screen->cols);

	int cols = Screen->cols;
	int rows = Screen->rows;

	// windows
	Screen->nav = newwin(rows, cols/5, 0, 0);
	Screen->display = newwin(rows-2, 4*rows-2, 0, cols/5);
	Screen->input = newwin(3, 4*cols/5, rows-3, cols/5);

	wlog("Windows created...");

	/* Creating tab around Screen->display and setting that to tabs[0] */
	Screen->tabs[0] = malloc(sizeof(struct tab));
	strncpy(Screen->tabs[0]->tabname, "default", sizeof(char) * 10);
	Screen->tabs[0]->win = Screen->display;

	/* Set display to current visible win */
	Screen->curtab = Screen->tabs[0];

	wlog("Tabs allocated...");

	refresh(); // don't know why this is necessary here

	// creating outlines for main windows
	box(Screen->nav, 0, 0);
	wrefresh(Screen->nav);

	box(Screen->display, 0, 0);
	wrefresh(Screen->display);

	box(Screen->input, 0, 0);
	wmove(Screen->input, 1, 1);
	wrefresh(Screen->input);

	// initializing more variables (related to created wins now though)
	getyx(Screen->display, Screen->dy, Screen->dx);
	getyx(Screen->nav, Screen->ny, Screen->nx);
	getmaxyx(Screen->nav, Screen->max_ny, Screen->max_nx);

	/* needs to be after Screen->max_nx is defined */
	show_tabs();

	/* This has no functional use, just makes things look nice */
	wmove(Screen->input, 1, 1);

	wlog("Screen initialization complete.");
}

void stop_screen()
{
	wlog("Shutting down screen...");
	delwin(Screen->nav);
	delwin(Screen->input);

	/* This will clean up Screen->display too because its at tabs[0] */
	for (int i = 0; i <= Screen->maxtab; i++) {
		if (Screen->tabs[i] != NULL) {
			deltab(Screen->tabs[i]);
		}
	}

	endwin();
	free(Screen);
	wlog("Screen shutdown complete.");
}
