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
extern char server[];

//
char buff[256]; // A block of memory to be used by all.
bool done = false;
bool console_state = false;

void help()
{
    cputs("Usage: yail [OPTIONS]\r\n");
    cputs("  -h this message\r\n");
    cputs("  -l <filename> load image file\r\n");
    cputs("  -u <url> use this server address\r\n");
    cputs("  -s <tokens> search terms\r\n");
}

void process_command_line(int argc, char* argv[])
{
    switch(argv[1][1])
    {
        case 'h':
            help();
            break;
        case 'l':
            fix_chars(argv[2]);
            load_image_file(argv[2]);
            break;
        case 'u':
            strcpy(server, argv[2]);
            break;
        case 's':
            stream_image(server, &argv[2]);
            break;
    }
}

//
int main(int argc, char* argv[])
{
    if(argc > 1)
    {
        process_command_line(argc, argv);
        cgetc();
        return 0;
    }
    else
    {
        // Clear the edit buffer so as not to confuse our console code.
        clrscr();

        // Initialize the frame buffer
        saveCurrentGraphicsState();
        setGraphicsMode(GRAPHICS_8);
        clearFrameBuffer();

        while(!done)
        {
            if(kbhit())
            {
                char ch = cgetc();
                if(ch == CH_ESC)
                    break;
                
                show_console();
                start_console(ch);
            }
        }

        setGraphicsMode(GRAPHICS_0);
        restoreGraphicsState();
        clrscr();
    }

    return 0;
}
