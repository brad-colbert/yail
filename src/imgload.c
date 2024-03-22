#include "console.h"
#include "graphics.h"
#include "files.h"
#include "utility.h"
#include "netimage.h"
#include "settings.h"
#include "version.h"
#include "utility.h"

#include <atari.h>
#include <conio.h>
#include <peekpoke.h>

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

//
char version[] = "YAIL (Yet Another Image Loader) v" TOSTR(MAJOR_VERSION) "." TOSTR(MINOR_VERSION) "." TOSTR(BUILD_VERSION);

byte buff[256]; // A block of memory to be used by all.
bool done = false;
extern Settings settings;
extern ushort ORIG_VBII_SAVE;

void help()
{
    cputs("Usage: yail [OPTIONS]\r\n");
          "  -h this message\r\n"
    #ifdef YAIL_BUILD_FILE_LOADER
          "  -l <filename> load image file\r\n"
    #endif
          "  -u <url> use this server address\r\n"
          "  -s <tokens> search terms\r\n";
}

void process_command_line(char* argv[])
{
    switch(argv[1][1])
    {
        case 'h':
            help();
            break;
        #ifdef YAIL_BUILD_FILE_LOADER
        case 'l':
            internal_to_atascii(argv[2], 40);
            load_image_file(argv[2]);
            break;
        #endif
        case 'u':
            strcpy(settings.url, argv[2]);
            break;
        case 's':
            stream_image(&argv[2], 0);
            break;
    }
}

//
int main(int argc, char* argv[])
{
    // Convert the version string to internal code format
    atascii_to_internal(version, 40);

    //
    if(argc > 1)
    {
        process_command_line(argv);
        return 0;
    }
    else
    {
        // Clear the edit buffer so as not to confuse our console code.
        clrscr();

        // Initialize the frame buffer
        saveCurrentGraphicsState();

        // Initialize the settings.  Set defaults if no saved settings are found.
        get_settings();
        clearFrameBuffer();

        // Stop the attract mode
        ORIG_VBII_SAVE = OS.vvblki;
        add_attract_disable_vbi();

        // Show the splash screen
        OS.sdmctl = 0x0;            // turn off the display
        stream_image(&argv[1], 1);  // get the splash screen
        OS.sdmctl = 0x22;           // turn on the display

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
                start_console(ch);   // send the character just entered so the console will start with it shown
            }
        }

        setGraphicsMode(GRAPHICS_0);
        restoreGraphicsState();
        clrscr();
    }

    return 0;
}
