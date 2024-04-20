// Copyright (C) 2021 Brad Colbert

#ifndef UTILITY_H
#define UTILITY_H

#include "types.h"

#define TOSTR_(x) #x
#define TOSTR(x) TOSTR_(x)

void pause(const char* message);
void internal_to_atascii(char* buff, byte len);
void atascii_to_internal(char* buff, byte len);
void show_error(const char* message);
void show_error_pause(const char* message);
extern void add_attract_disable_vbi();
extern void remove_attract_disable_vbi();
void wait_vbi(void);

#endif