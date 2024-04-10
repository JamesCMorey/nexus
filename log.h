#pragma once

#include "config.h"

void init_log();
void stop_log();
void wlog(char *text);
void walog(int type, char *text, const void *arg);
char *timestamp();
