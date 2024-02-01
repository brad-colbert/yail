// Copyright (C) 2021 Brad Colbert

#ifndef NETIMAGE_H
#define NETIMAGE_H

signed char enable_network(const char* url);
signed char disable_network(const char* url);
signed char check_network(const char* url);
signed char write_network(const char* url, const char* buf, unsigned short len);
signed char read_network(const char* url, unsigned char* buf, unsigned short len);

void loadImage(char* url, char* args[]);

#endif // NETIMAGE_H