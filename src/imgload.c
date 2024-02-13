//#define USE_TEST_CODE
#ifdef USE_TEST_CODE

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
#include <unistd.h>

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
    ushort i = 0;
    //const char search_terms[] = "search \"airplane piper comanche pa24\"";
    //const size_t search_length = sizeof(search_terms);

    saveCurrentGraphicsState();

    // init the frame buffer
    for(; i < FRAMEBUFFER_SIZE+32; ++i)
        framebuffer[i] = i%(ushort)256;

    #define LOAD_IMAGES
    #ifdef LOAD_IMAGES
    // Make sure the image data is pointing to the correct thing
    image.data = framebuffer;

    OS.soundr=0; // Turn off SIO beeping sound
    cursor(1);   // Keep cursor on

    // Clear the edit buffer so as not to confuse our console code.
    clrscr();

    OS.lmargn=0; // Set left margin to 0
    OS.shflok=0; // turn off shift-lock.

    // Attempt open.
    cprintf("Opening: %s\n\r", url);

    if (enable_network(url) < 0)
    {
        cprintf("Failed to open network\n\r");
        disable_network(url);
        return -1;
    }

    //if(write_network(url, "search apod astronomy", 12) < 0)  // Send the request
    if(write_network(url, "search \"airplane piper comanche pa24\"", 38) < 0)  // Send the request
    {
        cprintf("Unable to write request\n\r");
        disable_network(url);
        return -1;
    }

    pause("Press any key to load the image\n\r");

    //setGraphicsMode(image.header.gfx);
    setGraphicsMode(GRAPHICS_9);
    //show_console();

    #define READ_IMAGE_IN_BLOCKS
    #ifdef READ_IMAGE_IN_BLOCKS
    {
        bool done = false;
        const ushort bytes_per_line = 40;
        const ushort lines = 220;
        ushort buffer_start;
        ushort block_size;
        ushort lines_per_block;
        ushort dl_block_size;
        ushort ttl_buff_size;
        ushort read_size;

        while(!done)
        {
            if(kbhit())
            {
                cgetc();
                done = true;
                break;
            }
            else
            {
                buffer_start = framebuffer;
                block_size = DISPLAYLIST_BLOCK_SIZE;
                lines_per_block = (ushort)(block_size/bytes_per_line);
                dl_block_size = lines_per_block * bytes_per_line;
                ttl_buff_size = lines * bytes_per_line;
                read_size = dl_block_size;

                // Show loading messages
                //show_console();

                // Read the header
                if(read_network(url, (unsigned char*)&image.header, sizeof(image.header)) < 0)
                {
                    cprintf("Error reading\n\r");
                    disable_network(url);
                    return -1;
                }

                while(ttl_buff_size > 0)
                {
                    if(read_size > ttl_buff_size)
                        read_size = ttl_buff_size;

                    clrscr();
                    cprintf("Start %04X Read %04X\n\r", buffer_start, read_size);
                    if(read_network(url, buffer_start, read_size) < 0)
                    {
                        cprintf("Error reading\n\r");
                        disable_network(url);
                        return -1;
                    }

                    buffer_start = buffer_start + block_size;
                    ttl_buff_size = ttl_buff_size - read_size;
                }

                // Hide messages and show only the image
                //hide_console();

                sleep(5);

                if(write_network(url, "next", 4) < 0)  // Send the request
                {
                    cprintf("Unable to write request\n\r");
                    done = true;
                    break;
                }
            }
        }
    }

    #else

    if(read_network(url, image.data, sizeof(framebuffer)) < 0)
    {
        cprintf("Error reading\n\r");
        disable_network(url);
        return -1;
    }

    #endif

    if(write_network(url, "quit", 4) < 0)  // Send the request
        cprintf("Unable to write request\n\r");

    disable_network(url);

    OS.soundr = 3; // Restore SIO beeping sound
    #else

    setGraphicsMode(GRAPHICS_9);

    {
        bool console = false;
        while (1)
        {
            if(kbhit())
            {
                if(cgetc() == CH_ESC)
                    break;
                
                if(console)
                {
                    hide_console();
                    console = false;
                }
                else
                {
                    show_console();
                    console = true;
                }
            }
        }
    }

    pause("Press any key to continue\n\r");
    #endif

    // Show how it looks in Gfx0 (text mode)
    restoreGraphicsState();

    pause("Press any key to quit\n\r");

    return 0;
}

#else

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
#include <stdbool.h>

// Externs
extern byte framebuffer[]; 

//
char buff[256]; // A block of memory to be used by all.
bool done = false;
bool console_state = false;

//
int main(int argc, char* argv[])
{
    int i;

    // Clear the edit buffer so as not to confuse our console code.
    clrscr();

    // Initialize the frame buffer
    saveCurrentGraphicsState();
    setGraphicsMode(GRAPHICS_8);
    clearFrameBuffer();

    // init the frame buffer
    //for(i = 0; i < FRAMEBUFFER_SIZE+32; ++i)
    //    framebuffer[i] = i%(ushort)256;

    while(!done)
    {
        if(kbhit())
        {
            if(cgetc() == CH_ESC)
                break;
            
            show_console();
            start_console();
        }
    }

    setGraphicsMode(GRAPHICS_0);
    restoreGraphicsState();
    clrscr();

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
        show_console();

    console_update();
    hide_console();

    // Restore the graphics state back to the starting state.
    setGraphicsMode(GRAPHICS_0);
    restoreGraphicsState();
    clrscr();
    #endif

    return 0;
}

#endif