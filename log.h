#pragma once

char *timestamp();
void init_log();
void wlog(char *text);
void walog(int type, char *text, void *arg);
