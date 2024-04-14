#pragma once

#include "config.h"

void init_log();
void stop_log();
void wlog(char *text, ...);
char *timestamp();
