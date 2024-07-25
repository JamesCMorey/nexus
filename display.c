#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <stdarg.h>
#include <locale.h>
#include "display.h"
#include "commands.h"
#include "log.h"
#include "ds/ll.h"

#define CLEAR_WIN(win) werase(win); box(win, 0, 0)
#define REMOVE_LAST_CHAR(arr) arr[strlen(arr) - 1] = '\0'

#define MAX_TABNAME_LEN 10
#define MSGS_PER_TAB 16384
#define MAX_MSG_LEN 500
#define DISPLAY_WIDTH Screen->max_dx - 2

struct tab {
	int index; // used differentiate between tabs
	/* Name that is displayed in the nav bar for the tab */
	char tabname[MAX_TABNAME_LEN];
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

	linkedlist_t tabs; // tabs[0] is the default
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
static int display_msg(char *text);
static int count_msg_fill_display(void);

/* Handling tabs */
static void addmsg(struct tab *tb, char *text, va_list args);
static void size_addmsg(struct tab *tb, char *text, int size, va_list args);
static struct tab *ind_get_tab(int tab_index);

struct tab *get_tab(linkedlist_t tabs, int index);

/* ====== INPUT ====== */
int handle_input() /* TODO add check to ensure input is <500 chars */
{
	int rv = 0;
	char c;

	wlog("beginning to handle input.");

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
	wlog("In display");
	va_start(args, text);
	addmsg(Screen->curtab, text, args);
	va_end(args);

	display_tab(Screen->curtab);
}

static void display_tab(struct tab *tb)
{
	clr_display();
	Screen->curtab->y = 0;

	wlog("In display_tab");
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
			wlog("Msgnum: %d", msgnum);
			return msgnum;
		}

		/* count num rows each msg takes */
		if (getnewmsg) {
			finalindex = Screen->curtab->msgnum - 1;
			msglen = strlen(Screen->curtab->msgs[finalindex
								- msgnum]);
			getnewmsg = 0;
		}

		msglen -= (DISPLAY_WIDTH); /* -2 for border */

		/* increment only if the screen can hold the whole message */
		if (msglen <= 0) {
			msgnum++;
			getnewmsg = 1;
		}

		/* each iteration occupies a row */
		rows++;
	}

	wlog("Msgnum: %d", msgnum);
	return msgnum;
}

static int display_msg(char *text)
{
	int rv;
	rv = 0;

	char buf[DISPLAY_WIDTH - 1];
	int j = 0, empty = 1;
	for (int i = 0; i < strlen(text); i++) {
		if (text[i] == '\n') {
			wlog("Return exit of display_msg");
			buf[j] = '\0';
			displayln(buf);
			j = 0;
			rv++;
			empty = 1;
			continue;
		}

		if (i != 0 && i % (DISPLAY_WIDTH - 1) == 0) {
			buf[j] = text[i];
			buf[j + 1] = '\0';
			wlog("Normal exit of display_msg strlen: %d",
								strlen(buf));
			displayln(buf);
			j = 0;
			empty = 1;
		}

		wlog("Char: %c", text[i]);
		buf[j] = text[i];
		j++; /* This way incrementation is skipped on continue */
		empty = 0;
	}
	if (!empty) {
		buf[j] = '\0';
		displayln(buf);
	}

	return rv;
}

static void displayln(char *text)
{
	mvwprintw(Screen->display, ++Screen->curtab->y, 1, "%s", text);
}

void size_add_to_tab(int index, char *text, int size, ...)
{
	va_list args;

	va_start(args, size);
	size_addmsg(get_tab(Screen->tabs, index), text, size, args);
	va_end(args);
	if (get_tab(Screen->tabs, index) == Screen->curtab) {
		display_tab(Screen->curtab);
	}
}

void add_to_tab(int index, char *text, ...)
{
	va_list args;

	va_start(args, text);
	addmsg(get_tab(Screen->tabs, index), text, args);
	va_end(args);
	if (get_tab(Screen->tabs, index) == Screen->curtab) {
		display_tab(Screen->curtab);
	}
}

void add_to_default(char *text, ...)
{ /* Add ability to add to tab based off id */
	va_list args;

	va_start(args, text);
	addmsg(get_tab(Screen->tabs, 0), text, args);
	va_end(args);
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

/* ===== TABBING ====== */
/* create a tab and set it to Screen->curtab */
void mktab(char *name, int index)
{
	int i = index; /* make up for default at tabs[0] */
	struct tab *tb =  malloc(sizeof(struct tab));

	ll_add(Screen->tabs, (void*)tb);
	strncpy(tb->tabname, name, sizeof(char) * 10);
	tb->index = i;
	tb->y = 0;
	tb->x = 0;
	tb->msgnum = 0;

	Screen->curtab = tb;
	if (i > Screen->maxindex) {
		Screen->maxindex = i;
	}

	Screen->tabnum++; // TODO: replace tabnum with ll_len()

	display_tab(Screen->curtab);
	wlog("display_tab complete.");
	show_tabs();
	wlog("show_tabs complete.");
}

/* In case more things are added to struct tab */
void deltab(int index)
{
	struct tab *tb = get_tab(Screen->tabs, index);

	if (tb == NULL) {
		display("Cannot close tab (%d) as it doesn't exist.", index);
		return;
	}

	bool last_tab = true;
	if (!ll_point_at_tail(Screen->tabs)) {
		ll_next(Screen->tabs);
		last_tab = false;
	}

	if (!ll_point_at_head(Screen->tabs)) {
		ll_prev(Screen->tabs);
		last_tab = false;
	}

	if (last_tab) {
		display("At least one tab must remain open.");
		return;
	}


	ll_set_point(Screen->tabs, (void *)&index);
	ll_next(Screen->tabs);
	ll_del(Screen->tabs, (void *)&index);

	Screen->curtab = (struct tab*)ll_point(Screen->tabs);
	switch_tab(Screen->curtab->index); // rework this so ll_get runs once

	Screen->maxindex--;
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

static void size_addmsg(struct tab *tb, char *text, int size, va_list args)
{
	if (size < MAX_MSG_LEN) {
		vsnprintf(tb->msgs[tb->msgnum], size, text, args);
	}
	else {
		vsnprintf(tb->msgs[tb->msgnum], MAX_MSG_LEN, text, args);
	}
	tb->msgnum++;
}

static void addmsg(struct tab *tb, char *text, va_list args)
{
	vsnprintf(tb->msgs[tb->msgnum], MAX_MSG_LEN, text, args);
	tb->msgnum++;
}

static struct tab *ind_get_tab(int index)
{
	/* +1 to convert from tabindex to connindex conns[0] -> tabs[1] */
	if (get_tab(Screen->tabs, index) == NULL) {
		return NULL;
	}

	return get_tab(Screen->tabs, index);
}

int get_curtab_index(void)
{
	return Screen->curtab->index;
}

void show_tabs()
{
	clrwin(Screen->nav);
	Screen->ny = 0;
	wlog("in show_tabs");
	ll_reset_point(Screen->tabs);

	struct tab *tb;
	for (int i = 0; i < Screen->tabs->length; i++) {
		tb = (struct tab*)ll_point(Screen->tabs);
		if (!ll_point_at_tail(Screen->tabs))
			ll_next(Screen->tabs);
		char *name = tb->tabname;

		if (tb == NULL) {
			wlog("WARNING: tab in show_tabs found NULL.");
			continue;
		}

		wlog("loopin on: %s", name);
		if (tb == Screen->curtab) {
			wattrset(Screen->nav, A_BOLD);

			mvwprintw(	Screen->nav,
					++Screen->ny,
					(Screen->max_nx-strlen(name)-3)/2,
					"%d :%s",
					tb->index,
					name);

			wattroff(Screen->nav, A_BOLD);
			continue;
		}

		if (tb->unread == 1) {
			wattrset(Screen->nav, A_ITALIC);

			mvwprintw(	Screen->nav,
					++Screen->ny,
					(Screen->max_nx-strlen(name)-1)/2,
					"%d %s",
					tb->index,
					name);

			wattroff(Screen->nav, A_ITALIC);
			continue;
		}

		if (tb != NULL) {
			mvwprintw(	Screen->nav,
					++Screen->ny,
					(Screen->max_nx-strlen(name)-1)/2,
					"%d %s",
					tb->index,
					name);
		}
	}

	wrefresh(Screen->nav);
}

void switch_tab(int index)
{
	struct tab *tb = get_tab(Screen->tabs, index);
	if (tb == NULL) {
		wlog("cannot switch to nonexistent tab index");
		return;
	}

	Screen->curtab = tb;
	display_tab(Screen->curtab);
	show_tabs();
}

/* ====== INIT AND CLEANUP ====== */

void *tab_get_key(void *entry) {
	return (void*)&(((struct tab*)entry)->index);
}

bool tab_key_equiv(void *k1, void *k2) {
	return (*(int*)k1) == (*(int*)k2);
}

void tab_free_entry(void *entry) {
	free(entry);
}

struct tab *get_tab(linkedlist_t tabs, int index) {
	return ll_get(tabs, (void*)&index);
}

void init_screen(void)
{
	wlog("Initializing screen...");
	// Init functions
	setlocale(LC_CTYPE, "");
	initscr();
	cbreak();
	keypad(stdscr, TRUE);
	noecho();

	// initializing variables
	Screen = malloc(sizeof(struct screenState));
	Screen->maxindex = 0;
	Screen->tabnum = 0;
	Screen->tabs = ll_new(&tab_get_key, &tab_key_equiv, NULL);//&tab_free_entry);

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

	show_tabs();
	wmove(Screen->input, 1, 1);

	wlog("Screen initialization complete.");
}

void stop_screen(void)
{
	wlog("Shutting down screen...");
	delwin(Screen->nav);
	delwin(Screen->input);
	ll_free(Screen->tabs);

	endwin();
	free(Screen);
	wlog("Screen shutdown complete.");
}
