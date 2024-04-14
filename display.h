#pragma once

#include "config.h"

int curtab_textable();
int get_curtab_index(void);
int handle_input();
void display(char *text, ...);
void mktab(char *hostname, int sfd);
void switch_tab(int index);
void clr_display(void);
void init_screen();
void stop_screen();
void deltab(int index);
