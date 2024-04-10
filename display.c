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
	int id; /* 0 means local/not networked */
	WINDOW *win;
	int y, x;
};

struct screenState {
	int rows, cols;

	WINDOW *nav;
	int ny, nx;
	int max_ny, max_nx;

	WINDOW *input;
	int iy, ix;
	char buffer[1024]; /* input buffer from WINDOW *input */

	/* tabs organized with file descriptor such that tabs[sfd]; is the tab
	 * that is used to display communications to and from that socket. */
	struct tab *tabs[1025]; // tabs[0] is the default
	struct tab *curtab;
	WINDOW *curwin;
	int maxtab;
};

/* global struct to hold all information, windows, and input from the screen */
struct screenState *Screen;

static void clrwin(WINDOW *win);
static void deltab(struct tab *tb);
static void show_tabs(void);
static void add_to_tab(char *buffer);
void store_curwin();
static WINDOW *retrieve_win(int id);

/* INPUT */
int handle_input()
{
	int rv = 0;
	char c;

	c = wgetch(Screen->input);
	strncat(Screen->buffer, &c, 1);

	/* Command handling */
	if (c == '\n' && Screen->buffer[0] == ':') {
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
		mvwprintw(Screen->curwin, ++Screen->curtab->y, 1, "%s", text);
		break;

	case INT:
		mvwprintw(Screen->curwin, ++Screen->curtab->y, 1, text,
							*(int *)arg);
		break;

	case STR:
		mvwprintw(Screen->curwin, ++Screen->curtab->y, 1, text,
							(char *)arg);
		break;

	default:
		mvwprintw(Screen->curwin, ++Screen->curtab->y, 1,
			"Type not supported in call to display(): %s, %d",
			__FILE__, __LINE__);
	}

	wrefresh(Screen->curwin);
}

void clr_cur_win()
{
	clrwin(Screen->curwin);
}

static void clrwin(WINDOW *win)
{
	werase(win);
	box(win, 0, 0);
	wrefresh(win);
}

/* ===== TABBING ====== */
void mktab(char *name, int id)
{
	int i = id;

	while (Screen->tabs[i] != NULL) {
		i++;
	}
	Screen->tabs[i] = malloc(sizeof(struct tab));
	strncpy(Screen->tabs[i]->tabname, name, sizeof(char) * 10);
	Screen->tabs[i]->win = newwin(Screen->rows-2, 4*Screen->rows-2, 0,
								Screen->cols/5);
	Screen->tabs[i]->id = id; /* Allows matching network conn to tab */

	if (i > Screen->maxtab) {
		Screen->maxtab = i;
	}
	clrwin(Screen->tabs[i]->win);
	switch_tab(id);
	getyx(Screen->curwin, Screen->curtab->y, Screen->curtab->x);
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
		char *name = Screen->tabs[i]->tabname;
		if (Screen->tabs[i] == Screen->curtab) {
			wattrset(Screen->nav, A_BOLD);
			mvwprintw(Screen->nav, ++Screen->ny,
				(Screen->max_nx-strlen(name)-3)/2, "%d :%s",
						Screen->tabs[i]->id, name);
			wattroff(Screen->nav, A_BOLD);
			continue;
		}

		if (Screen->tabs[i]->unread == 1) {
			wattrset(Screen->nav, A_ITALIC);
			mvwprintw(Screen->nav, ++Screen->ny,
				(Screen->max_nx-strlen(name)-1)/2, "%d %s",
						Screen->tabs[i]->id, name);

			wattroff(Screen->nav, A_ITALIC);
			continue;
		}

		if (Screen->tabs[i] != NULL) {
			mvwprintw(Screen->nav, ++Screen->ny,
				(Screen->max_nx-strlen(name)-1)/2, "%d %s",
						Screen->tabs[i]->id, name);
		}
	}
	wrefresh(Screen->nav);
}

void switch_tab(int id)
{
	for (int i = id; i <= Screen->maxtab; i++) {
		display(INT, "i: %d", &i);
		if (Screen->tabs[i] != NULL && Screen->tabs[i]->id == id) {
			clrwin(Screen->curtab->win);

			Screen->curtab = Screen->tabs[i];
			Screen->curwin = Screen->tabs[i]->win;

			display(INT, "Switching to %d.", &Screen->tabs[i]->id);
			wrefresh(Screen->curwin);
			show_tabs();
			return;
		}
	}

	display(INT, "Unable to find tab of id: %d", &id);
}

void store_curwin()
{
	char filename[10];
	char mode = 'w';
	FILE *fd;

	sprintf(filename, ".%d.win", Screen->curtab->id);
	fd = fopen(filename, &mode);

	putwin(Screen->curtab->win, fd);
	clrwin(Screen->curtab->win);
	//delwin(Screen->curtab->win);
}

static WINDOW *retrieve_win(int id)
{
	char filename[10];
	char mode = 'w';
	FILE *fd;

	sprintf(filename, ".%d.win", id);
	fd = fopen(filename, &mode);

	return getwin(fd);
}

static void add_to_tab(char *buffer) {

}

/* ====== INIT AND CLEANUP ====== */
void init_screen(void)
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
	Screen->input = newwin(3, 4*cols/5, rows-3, cols/5);

	wlog("Windows created...");

	/* Creating tab around Screen->display and setting that to tabs[0] */
	Screen->tabs[0] = malloc(sizeof(struct tab));
	strncpy(Screen->tabs[0]->tabname, "default", sizeof(char) * 10);
	Screen->tabs[0]->win = newwin(rows-2, 4*rows-2, 0, cols/5);
	Screen->curwin = Screen->tabs[0]->win;

	/* Set display to current visible win */
	Screen->curtab = Screen->tabs[0];

	wlog("Tabs allocated...");

	refresh(); // don't know why this is necessary here

	// creating outlines for main windows
	box(Screen->nav, 0, 0);
	wrefresh(Screen->nav);

	box(Screen->curwin, 0, 0);
	wrefresh(Screen->curwin);

	box(Screen->input, 0, 0);
	wmove(Screen->input, 1, 1);
	wrefresh(Screen->input);

	// initializing more variables (related to created wins now though)
	getyx(Screen->curwin, Screen->curtab->y, Screen->curtab->x);
	getyx(Screen->nav, Screen->ny, Screen->nx);
	getmaxyx(Screen->nav, Screen->max_ny, Screen->max_nx);

	/* needs to be after Screen->max_nx is defined */
	show_tabs();

	/* This has no functional use, just makes things look nice */
	wmove(Screen->input, 1, 1);

	wlog("Screen initialization complete.");
}

void stop_screen(void)
{
	wlog("Shutting down screen...");
	delwin(Screen->nav);
	delwin(Screen->input);

	for (int i = 0; i <= Screen->maxtab; i++) {
		if (Screen->tabs[i] != NULL) {
			deltab(Screen->tabs[i]);
		}
	}

	endwin();
	free(Screen);
	wlog("Screen shutdown complete.");
}
