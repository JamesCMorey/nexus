#pragma once

enum ARGTYPE {
	NOARG, INT, STR
};

void display(int type, char *text, char *arg);
int handle_input(char *buffer);
void init_display();
int stop_display();
