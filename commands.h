#pragma once

#include "net.h"

void handle_normal(char *text);
int handle_command(char *buffer);
int parse_commands(char *buffer);
