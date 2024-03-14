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

#endif