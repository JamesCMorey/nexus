#include <string.h>
#include "display.h"
#include "net.h"

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
	wrefresh(wins.input);

	return wins;
}

int stop_display(struct winfo wins)
{
	delwin(wins.nav);
	delwin(wins.display);
	delwin(wins.input);
	endwin();
}

int handle_input(const struct winfo wins, char *buffer)
{
	char c;
	int dx, dy, rv;
	getyx(wins.display, dy, dx);
	mvwprintw(wins.input, 1, 1, "%s", buffer);
	c = wgetch(wins.input);
	strncat(buffer, &c, 1);


	if (c == '\n' && buffer[0] == '/') {
		rv = parse_commands(buffer);

		if(rv == -1) {
			mvwprintw(wins.display, ++dy, 1, "Command not found:");
			wrefresh(wins.display);
		}
		else if (rv == EXIT) {
			return -1;
		}
	}

	if (c == KEY_BACKSPACE || c == KEY_DC || c == 127) {
		// need to delete DEL char and intended char
		if (strlen(buffer) > 0) {
			buffer[strlen(buffer) - 1] = '\0';
		}
		buffer[strlen(buffer) - 1] = '\0';
	}

	if (c == '\n') {
		buffer[strlen(buffer) - 1] = '\0';
		mvwprintw(wins.display, ++dy, 1, "%s", buffer);
		wrefresh(wins.display);

		memset(buffer, 0, sizeof(buffer));
	}

	werase(wins.input);
	box(wins.input, 0, 0);
	return 0;
}

int parse_commands(char *buffer)
{
	for (int i = 0; i < NUMCOMMANDS; i++) {
		if (!strncmp(&buffer[1], COMMANDS[i], strlen(COMMANDS[i]))) {
			return i;
		}
	}

	return -1;
}
