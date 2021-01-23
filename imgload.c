// Copyright (C) 2021 Brad Colbert

#include "console.h"
#include "graphics.h"

#include <atari.h>
#include <conio.h>
#include <peekpoke.h>

//
int main()
{
    /*
    unsigned int memlo = PEEKW(743);
    cprintf("%04X\r\n", memlo);
    cgetc();
    return 0;
    */

    // Clear the text and memory
    enable_console();
    cursor(1);
    reset_console();

    save_current_graphics_state();
    
    graphics_clear();
    set_graphics(GRAPHICS_8);  // Start in Gfx 8 for giggles

    console_update();

    restore_graphics_state();

    return 0;
}