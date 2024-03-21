// Copyright (C) 2021 Brad Colbert

#include "utility.h"
#include "graphics.h"
#include "types.h"

#include <conio.h>
#include <stdbool.h>
#include <atari.h>

// Globals
ushort ORIG_VBII_SAVE;// = OS.vvblki;

void pause(const char* message)
{
    if(message)
        cputs(message);

    while(true)
    {
        if(kbhit())
        {
            cgetc();  // flush the key press
            break;
        }
    }
}

void internal_to_atascii(char* buff, byte len)
{
    byte i;
    for(i=0;i<len;++i)
    {
        if(buff[i])  // leave the null terminator
            if(buff[i] < 64)
                buff[i] += 32;
            else if(buff[i] < 96)
                buff[i] -= 64;
            // otherwise leave it alone
    }
}

void atascii_to_internal(char* buff, byte len)
{
    byte i;
    for(i=0;i<len;++i)
    {
        if(buff[i])  // leave the null terminator
            if(buff[i] < 32)
                buff[i] += 64;
            else if(buff[i] < 96)
                buff[i] -= 32;
            // otherwise leave it alone
    }
}

void show_error(const char* message)
{
    show_console();
    cputs(message);
}