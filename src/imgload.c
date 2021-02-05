// Copyright (C) 2021 Brad Colbert

#include "console.h"
#include "graphics.h"
#include "files.h"
#include "utility.h"

#include <atari.h>
#include <conio.h>
#include <peekpoke.h>

#include <stdlib.h>
#include <string.h>

//
extern GfxDef gfxState;

//
int main(int argc, char* argv[])
{
    byte i = 0;

    // Clear the edit buffer so as not to confuse our console code.
    clrscr();

    // Initialize everything
    memset(&gfxState, 0, sizeof(GfxDef));

    gfxState.dl.address = NULL;  // make sure it gets created
    gfxState.mode = 0xFF;        // make sure the mode get's set

    saveCurrentGraphicsState();
    setGraphicsMode(GRAPHICS_8);
    clearFrameBuffer();

    if(argc > 1)
        loadFile(argv[1]);

    else
        // Start user input.
        enableConsole();

    console_update();
    disableConsole();

    // Restore the graphics state back to the starting state.
    setGraphicsMode(GRAPHICS_0);
    restoreGraphicsState();
    clrscr();

    return 0;
}
