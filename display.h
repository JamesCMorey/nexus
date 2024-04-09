#pragma once

enum ARGTYPE {
	NOARG, INT, STR
};

void display(int type, char *text, void *arg);
int handle_input();
void init_display();
int stop_display();
