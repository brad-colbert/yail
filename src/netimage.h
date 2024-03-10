// Copyright (C) 2021 Brad Colbert

#ifndef NETIMAGE_H
#define NETIMAGE_H

#include "types.h"

// Defines
#define CONSOLE_LINES 5
#define CONSOLE_HIDDEN 0
#define CONSOLE_SHOWN 1
#define MAX_LINE_LEN 40

signed char enable_network(const char* url);
signed char disable_network(const char* url);
signed char check_network(const char* url);
signed char write_network(const char* url, const char* buf, unsigned short len);
signed char read_network(const char* url, unsigned char* buf, unsigned short len);
char stream_image(char* args[]);
void show_image(char* args[]);

#endif // NETIMAGE_H