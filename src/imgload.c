#include "console.h"
#include "graphics.h"
#include "files.h"
#include "utility.h"
#include "netimage.h"
//#include "atascii.h"

#include <atari.h>
#include <conio.h>
#include <peekpoke.h>

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Externs
extern char server[];

//
//char version[] = "YAIL (Yet Another Image Loader) v1.2.2";
const byte version[] = "\x00\x39\x21\x29\x2C\x00\x08\x39\x65\x74\x00\x21\x6E\x6F\x74\x68\x65\x72\x00\x29\x6D\x61\x67\x65\x00\x2C\x6F\x61\x64\x65\x72\x09\x00\x76\x11\x0E\x12\x0E\x12\x00";
char buff[256]; // A block of memory to be used by all.
bool done = false;

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
    //
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

        // Show console on startup
        show_console();
        start_console(0x00);

        while(!done)
        {
            if(kbhit())
            {
                char ch = cgetc();
                if(ch == CH_ESC)
                    break;
                if(ch == CH_ENTER)
                    ch = 0x00;
                
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
