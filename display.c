#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <stdarg.h>
#include "display.h"
#include "commands.h"
#include "log.h"

#define CLEAR_WIN(win) werase(win); box(win, 0, 0)
#define REMOVE_LAST_CHAR(arr) arr[strlen(arr) - 1] = '\0'

#define MAX_TABNAME_LEN 10
#define MSGS_PER_TAB 16384
#define MAX_MSG_LEN 500

struct tab {
	/* Name that is displayed in the nav bar for the tab */
	char tabname[MAX_TABNAME_LEN];
	int index;
	int unread:1; /* Flag for unseen activity in tab */

	char msgs[MSGS_PER_TAB][MAX_MSG_LEN];
	int msgnum;

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

	WINDOW *display;
	int max_dy, max_dx;
	int darea;

	/* tabs organized with file descriptor such that tabs[sfd]; is the tab
	 * that is used to display communications to and from that socket. */
	struct tab *tabs[1024]; // tabs[0] is the default
	struct tab *curtab;
	int maxindex;
	int tabnum;
};

/* global struct to hold all information, windows, and input from the screen */
static struct screenState *Screen;

/* Handling display */
static void clrwin(WINDOW *win);
static void displayln(char *text);
static void display_tab(struct tab *tb);
static void display_msg(char *text);
static int count_msg_fill_display(void);

/* Handling tabs */
static void addmsg(struct tab *tb, char *text, va_list args);
static struct tab *ind_get_tab(int tab_index);

/* ====== INPUT ====== */
int handle_input() /* TODO add check to ensure input is <500 chars */
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
		handle_normal(Screen->buffer);
		memset(Screen->buffer, 0, sizeof(Screen->buffer));
	}

	/* Backspace handling */
	if (c == KEY_BACKSPACE || c == KEY_DC || c == 127) {
		// need to delete DEL char and intended char
		if (strlen(Screen->buffer) > 0) {
			REMOVE_LAST_CHAR(Screen->buffer);
		}
		REMOVE_LAST_CHAR(Screen->buffer);
	}

	/* Clear the input field and redraw it, including the box around it. */
	clrwin(Screen->input);
	mvwprintw(Screen->input, 1, 1, "%s", Screen->buffer);
	wrefresh(Screen->input); /* refresh input */

	return rv;
}

/* ===== DISPLAY ====== */
void display(char *text, ...)
{ /* Add ability to add to tab based off id */
	va_list args;

	va_start(args, text);
	addmsg(Screen->curtab, text, args);
	va_end(args);

	display_tab(Screen->curtab);
}

void add_to_default(char *text, ...)
{ /* Add ability to add to tab based off id */
	va_list args;

	va_start(args, text);
	addmsg(Screen->tabs[0], text, args);
	va_end(args);
}

static void display_tab(struct tab *tb)
{
	clr_display();
	Screen->curtab->y = 0;

	int msgcount = count_msg_fill_display();
	int msgindex;
	for (int i = msgcount; i > 0; i--) { /* index 0 is the final message */
		msgindex = Screen->curtab->msgnum - i;
		display_msg(Screen->curtab->msgs[msgindex]);
	}

	wrefresh(Screen->display);
}

static int count_msg_fill_display(void)
{
	int rows, msgnum, finalindex, msglen, getnewmsg;

	if (Screen->curtab->msgnum == 0) {
		return -1;
	}

	rows = 0;
	msgnum = 0;
	getnewmsg = 1;
	/* while there is still space on screen
	 * && msgnum < Screen->curtab->msgnum */
	while (rows < (Screen->max_dy - 2)) {
		if (msgnum == Screen->curtab->msgnum) {
			return msgnum;
		}

		/* count num rows each msg takes */
		if (getnewmsg) {
			finalindex = Screen->curtab->msgnum - 1;
			msglen = strlen(Screen->curtab->msgs[finalindex
								- msgnum]);
			getnewmsg = 0;
		}

		msglen -= (Screen->max_dx - 2); /* -2 for border */

		/* increment only if the screen can hold the whole message */
		if (msglen <= 0) {
			msgnum++;
			getnewmsg = 1;
		}

		/* each iteration occupies a row */
		rows++;
	}

	return msgnum;
}

void display_msg(char *text)
{
	int lenprinted;
	int i = 0;
	char substring[Screen->max_dx - 2 + 1];
	int tmp = sizeof(substring);
	wlog("sizeof(substring): %d", tmp);

	lenprinted = (Screen->max_dx - 2);
	while(lenprinted == (Screen->max_dx - 2)) {
		strncpy(substring, text + i, sizeof(substring));
		substring[Screen->max_dx - 2] = '\0';

		displayln(substring);

		/* If lenprintd is less than the width, that means the msg
		 * has been fully printed */
		lenprinted = strlen(substring);
		i += Screen->max_dx - 2;
	}
}

static void displayln(char *text)
{
	mvwprintw(Screen->display, ++Screen->curtab->y, 1, "%s", text);
}

void clr_display(void)
{
	clrwin(Screen->display);
}

static void clrwin(WINDOW *win)
{
	werase(win);
	box(win, 0, 0);
}

/*
void quiet_display()
{
}
*/

/* ===== TABBING ====== */
/* create a tab and set it to Screen->curtab */
void mktab(char *name, int index)
{
	int i = index; /* make up for default at tabs[0] */
	struct tab *tb =  malloc(sizeof(struct tab));

	Screen->tabs[i] = tb;
	strncpy(Screen->tabs[i]->tabname, name, sizeof(char) * 10);
	Screen->tabs[i]->index = i;
	Screen->tabs[i]->y = 0;
	Screen->tabs[i]->x = 0;
	Screen->tabs[i]->msgnum = 0;

	Screen->curtab = Screen->tabs[i];
	if (i > Screen->maxindex) {
		Screen->maxindex = i;
	}

	Screen->tabnum++;

	display_tab(Screen->curtab);
	show_tabs();
}

/* In case more things are added to struct tab */
void deltab(int index)
{
	/* -1 so it doesn't just select the tab being deleted */
	for (int i = index - 1; i >= 0; i--) {
		if (index == Screen->maxindex && Screen->tabs[i] != NULL) {
			Screen->maxindex = i;
		}

		if (Screen->tabs[i] != NULL) {
			Screen->curtab = Screen->tabs[i];
			switch_tab(i);
			break;
		}
	}

	free(Screen->tabs[index]);
	Screen->tabs[index] = NULL;
}

int curtab_textable()
{
	 enum ConnType type = get_conntype(Screen->curtab->index);
	 if (type == -1) {
		 wlog("Error in curtab_textable: index %d not found ",
		 					Screen->curtab->index);

	 }

	 if (type == IRC || type == TCP) {
		 return 1;
	 }

	 return 0;
}

void set_tab_unread(struct tab *tb)
{

}

static void addmsg(struct tab *tb, char *text, va_list args)
{
	vsnprintf(tb->msgs[tb->msgnum], MAX_MSG_LEN, text, args);
	tb->msgnum++;
}

static struct tab *ind_get_tab(int index)
{
	/* +1 to convert from tabindex to connindex conns[0] -> tabs[1] */
	if (Screen->tabs[index] == NULL) {
		return NULL;
	}

	return Screen->tabs[index];
}

int get_curtab_index(void)
{
	return Screen->curtab->index;
}

void show_tabs()
{
	clrwin(Screen->nav);
	Screen->ny = 0;

	for (int i = 0; i <= Screen->maxindex; i++) {
		char *name = Screen->tabs[i]->tabname;
		if (Screen->tabs[i] == Screen->curtab) {
			wattrset(Screen->nav, A_BOLD);
			mvwprintw(Screen->nav, ++Screen->ny,
				(Screen->max_nx-strlen(name)-3)/2, "%d :%s",
						Screen->tabs[i]->index, name);
			wattroff(Screen->nav, A_BOLD);
			continue;
		}

		if (Screen->tabs[i]->unread == 1) {
			wattrset(Screen->nav, A_ITALIC);
			mvwprintw(Screen->nav, ++Screen->ny,
				(Screen->max_nx-strlen(name)-1)/2, "%d %s",
						Screen->tabs[i]->index, name);

			wattroff(Screen->nav, A_ITALIC);
			continue;
		}

		if (Screen->tabs[i] != NULL) {
			mvwprintw(Screen->nav, ++Screen->ny,
				(Screen->max_nx-strlen(name)-1)/2, "%d %s",
						Screen->tabs[i]->index, name);
		}
	}
	wrefresh(Screen->nav);
}

void switch_tab(int index)
{
	for (int i = index; i <= Screen->maxindex; i++) {
		if (Screen->tabs[i]->index == index) {
			Screen->curtab = Screen->tabs[i];
			display_tab(Screen->curtab);
			show_tabs();
			break;
		}
	}
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
	Screen->maxindex = 0;
	Screen->tabnum = 0;
	getmaxyx(stdscr, Screen->rows, Screen->cols);

	int cols = Screen->cols;
	int rows = Screen->rows;

	/* windows and default tab */
	Screen->nav = newwin(rows, cols/5, 0, 0);
	Screen->input = newwin(3, 4*cols/5, rows-3, cols/5);
	Screen->display = newwin(rows-2, 4*cols/5, 0, cols/5);

	mktab("default", 0);
	wlog("Windows and default tab initialized...");

	/* window info */
	getyx(Screen->nav, Screen->ny, Screen->nx);
	getmaxyx(Screen->nav, Screen->max_ny, Screen->max_nx);

	getyx(Screen->display, Screen->curtab->y, Screen->curtab->x);
	getmaxyx(Screen->display, Screen->max_dy, Screen->max_dx);
	Screen->darea = Screen->max_dy * Screen->max_dy;

	wlog("Screen struct variables initialized...");

	/* Displaying everything */
	box(Screen->nav, 0, 0);
	wrefresh(Screen->nav);

	box(Screen->display, 0, 0);
	wrefresh(Screen->display);

	box(Screen->input, 0, 0);
	wmove(Screen->input, 1, 1);
	wrefresh(Screen->input);

	wmove(Screen->input, 1, 1);
	show_tabs();

	wlog("Screen initialization complete.");
}

void stop_screen(void)
{
	wlog("Shutting down screen...");
	delwin(Screen->nav);
	delwin(Screen->input);

	for (int i = 0; i <= Screen->maxindex; i++) {
		if (Screen->tabs[i] != NULL) {
			deltab(i);
		}
	}

	endwin();
	free(Screen);
	wlog("Screen shutdown complete.");
}
