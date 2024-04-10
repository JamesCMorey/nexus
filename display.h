#pragma once

#include "config.h"

int handle_input();
void display(int type, char *text, const void *arg);
void mktab(char *hostname, int sfd);
void switch_tab(int id);
void clr_cur_win();
void init_screen();
void stop_screen();
