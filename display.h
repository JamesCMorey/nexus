#pragma once

enum ARGTYPE {
	NOARG, INT, STR
};

void display(int type, char *text, const void *arg);
int handle_input();
void init_screen();
void stop_screen();
