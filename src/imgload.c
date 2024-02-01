// Copyright (C) 2021 Brad Colbert
#include "types.h"
#include "netimage.h"
#include "graphics.h"

#include <atari.h>
#include <conio.h>
#include <peekpoke.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

//
struct image_header
{
    unsigned char v1;
    unsigned char v2;
    unsigned char v3;
    unsigned char gfx;
    unsigned char memtkn;
    short size;
};

//
struct image_data
{
    struct image_header header;

    //unsigned char data[8800];
    byte* data;
};

//
extern byte framebuffer[FRAMEBUFFER_SIZE];

//
char url[] = "N:TCP://192.168.1.205:9999/";                  // URL
char tmp[8];                    // temporary # to string
struct image_data image;

int main(int argc, char* argv[])
{
    unsigned short i = 0;

    saveCurrentGraphicsState();

    // init the frame buffer
    for(; i < FRAMEBUFFER_SIZE; ++i)
        framebuffer[i] = i%(ushort)256;

    // Make sure the image data is pointing to the correct thing
    image.data = framebuffer;

    OS.soundr=0; // Turn off SIO beeping sound
    cursor(1);   // Keep cursor on

    // Clear the edit buffer so as not to confuse our console code.
    clrscr();

    OS.lmargn=0; // Set left margin to 0
    OS.shflok=0; // turn off shift-lock.

    pause("Press any key to start\n\r");

    setGraphicsMode(GRAPHICS_9);

    pause(NULL);

    enableConsole();

    pause("Press any key to continue\n\r");

    #if 0
    // Attempt open.
    cprintf("Opening: %s\n\r", url);

    if (enable_network(url) < 0)
    {
        cprintf("Failed to open network\n\r");
        disable_network(url);
        return -1;
    }

    if(write_network(url, "search funny", 12) < 0)  // Send the request
    {
        cprintf("Unable to write request\n\r");
        disable_network(url);
        return -1;
    }

    if(read_network(url, (unsigned char*)&image.header, sizeof(image.header)) < 0)
    {
        cprintf("Error reading\n\r");
        disable_network(url);
        return -1;
    }

    if(read_network(url, image.data, sizeof(framebuffer)) < 0)
    {
        cprintf("Error reading\n\r");
        disable_network(url);
        return -1;
    }

    if(write_network(url, "quit", 4) < 0)  // Send the request
    {
        cprintf("Unable to write request\n\r");
        disable_network(url);
        return -1;
    }

    disable_network(url);

    OS.soundr = 3; // Restore SIO beeping sound

    // Print information about the image
    cprintf("ver %d.%d.%d  gfx %d  mtkn %d (%d)\n\r", image.header.v1, image.header.v2, image.header.v3, image.header.gfx, image.header.memtkn, image.header.size);

    // The first 10 bytes
    for(i = 0; i < 10; ++i)
    {
        cprintf("%02X ", image.data[i]);
    }
    cprintf("\n\r");

    // The last 10 bytes
    for(i = 8790; i < 8800; ++i)
    {
        cprintf("%02X ", image.data[i]);
    }
    cprintf("\n\r");
    #endif

    // Show how it looks in Gfx0 (text mode)
    restoreGraphicsState();
    //setGraphicsMode(GRAPHICS_0);

    pause("Press any key to quit\n\r");

    return 0;
}

#if 0

#include "console.h"
#include "graphics.h"
#include "files.h"
#include "utility.h"
#include "netimage.h"

#include <atari.h>
#include <conio.h>
#include <peekpoke.h>

#include <stdlib.h>
#include <string.h>

//
extern GfxDef gfxState;
extern char server[80];
const char* terms[] = {"funny", 0x0};

//
int main(int argc, char* argv[])
{
    byte i = 0;

    // Clear the edit buffer so as not to confuse our console code.
    clrscr();

    // Initialize everything
    memset(&gfxState, 0, sizeof(GfxDef));

    loadImage(server, terms);

    #if 0
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
    #endif

    return 0;
}

#endif