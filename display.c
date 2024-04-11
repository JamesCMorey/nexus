#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
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
	int id;
	int sfd;
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
	struct tab *tabs[1025]; // tabs[0] is the default
	struct tab *curtab;
	int maxtab;
};

/* global struct to hold all information, windows, and input from the screen */
static struct screenState *Screen;

/* Handling display */
void display(int type, char *text, const void *arg); /* TODO remove this */
static void clrwin(WINDOW *win);
static void show_tabs(void);
static void displayln(char *text);
static void display_tab(struct tab *tb);
void display_msg(char *text);

/* Handling tabs */
static void deltab(struct tab *tb);
static void addmsg(struct tab *tb, int type, char *text, const void *arg);

/* Misc */
static int count_msg_fill_display(void);

/* INPUT */
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
{ /* Add ability to add to tab based off id */
	addmsg(Screen->curtab, type, text, arg);
	display_tab(Screen->curtab);
}
static void display_tab(struct tab *tb)
{
	clr_display();
	Screen->curtab->y = 0;
	/* STEPS
	 * 0. clrscreen
	 * 1. Count num of msgs that will fill the screen
	 * 2. store the extra bytes that are potentially there (partial msg)
	 * 3. Print the partial message at the top of the tab
	 * 4. Print the rest of the messages
	 * 5. wrefresh()
	 * */
	int msgcount = count_msg_fill_display();
	int msgindex;

	for (int i = msgcount; i > 0; i--) { /* index 0 is the final message */
		msgindex = Screen->curtab->msgnum - i;
		walog(INT, "Printing msg #%d", &msgindex);
		walog(STR, "Message: %s", Screen->curtab->msgs[msgindex]);
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
	/* while there is still space on screen */
	while (rows < Screen->max_dy) { /* && msgnum < Screen->curtab->msgnum */
		if (msgnum == Screen->curtab->msgnum) {
			return msgnum;
		}

		/* count num rows each msg takes */
		if (getnewmsg) {
			finalindex = Screen->curtab->msgnum - 1;
			msglen = strlen(Screen->curtab->msgs[finalindex - msgnum]);
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
	walog(INT, "sizeof(substring): %d", &tmp);

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
	// wrefresh(Screen->display); /* TODO reorganize this */
}

void clr_display(void)
{
	clrwin(Screen->display);
}

static void clrwin(WINDOW *win)
{
	werase(win);
	box(win, 0, 0);
	wrefresh(win);
}

/* ===== TABBING ====== */

static void addmsg(struct tab *tb, int type, char *text, const void *arg)
{
	switch(type) {
	case NOARG:
		snprintf(tb->msgs[tb->msgnum], MAX_MSG_LEN, "%s", text);
		break;

	case INT:
		snprintf(tb->msgs[tb->msgnum], MAX_MSG_LEN, text, *(int *)arg);
		break;

	case STR:
		snprintf(tb->msgs[tb->msgnum], MAX_MSG_LEN, text, (char *)arg);
		break;

	default:
		snprintf(tb->msgs[tb->msgnum], MAX_MSG_LEN,
			"Type not supported in call to display(): %s, %d",
			__FILE__, __LINE__);
	}

	tb->msgnum++;
}

/* create a tab and set it to Screen->curtab */
void mktab(char *name, int sfd)
{
	int i;
	if (sfd > 3) { /* Make up for default fds stdin, stdout, stderr */
		i = sfd - 3;
	}
	else {
		i = sfd;
	}

	while (Screen->tabs[i] != NULL) {
		i++;
	}

	Screen->tabs[i] = malloc(sizeof(struct tab));
	strncpy(Screen->tabs[i]->tabname, name, sizeof(char) * 10);
	Screen->tabs[i]->id = i;
	Screen->tabs[i]->sfd = sfd; /* Allows matching network conn to tab */
	Screen->tabs[i]->y = 0;
	Screen->tabs[i]->x = 0;
	Screen->tabs[i]->msgnum = 0;

	// getyx(Screen->display, Screen->curtab->y, Screen->curtab->x);

	Screen->curtab = Screen->tabs[i];
	if (i > Screen->maxtab) {
		Screen->maxtab = i;
	}

	display_tab(Screen->curtab);
	show_tabs();
}

/* In case more things are added to struct tab */
static void deltab(struct tab *tb)
{
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
		if (Screen->tabs[i]->id == id) {
			Screen->curtab = Screen->tabs[i];
			display_tab(Screen->curtab);
			show_tabs();
			break;
		}
	}
}

/*
void quiet_display()
{
}
*/


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

	for (int i = 0; i <= Screen->maxtab; i++) {
		if (Screen->tabs[i] != NULL) {
			deltab(Screen->tabs[i]);
		}
	}

	endwin();
	free(Screen);
	wlog("Screen shutdown complete.");
}
