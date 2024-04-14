#pragma once

#include "config.h"

/* ====== INPUT ======*/
int handle_input();

/* ====== DISPLAY ====== */
void display(char *text, ...);
void add_to_default(char *text, ...);
void add_to_tab(int index, char *text, ...);
void clr_display(void);

/* ====== TABBING  ====== */
void mktab(char *hostname, int sfd);
void deltab(int index);
void show_tabs();
void switch_tab(int index);
int curtab_textable();
int get_curtab_index(void);

/* ====== INIT && SHUTDOWN  ====== */
void init_screen();
void stop_screen();
