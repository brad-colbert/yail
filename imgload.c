// Copyright (C) 2021 Brad Colbert

#include <conio.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#include "readNetPBM.h"
#include "console.h"
#include "graphics.h"
#include "displaylist.h"
#include "types.h"
#include "consts.h"

#define GR_8

#ifdef GR_8
#define FILENAME "MTFUJID.PBM"
#else
#define FILENAME "MTFUJI.PGM"
#endif

// Globals
int fd;

// Externs
extern byte ORG_GFX_STATE;
extern unsigned ORG_DLIST;
extern byte NMI_STATE;
extern byte console_state;

//
int main()
{
    // Clear the text and memory
    clrscr();
    memset((void*)MY_SCRN_MEM, 0x00, 0x3000);

    save_current_graphics_state();
    
    enable_console();
    
    set_graphics(GRAPHICS_9);
    
    gotoxy(0,0);
    cursor(1);

    console_update();
    /*
    fd = open(FILENAME, O_RDONLY);
    if(fd >= 0)
    {
        readPBMIntoGfx8(fd, (void*)MY_SCRN_MEM);
    }
    close(fd);

    cgetc();
    */

    restore_graphics_state();

    #if 0
    char input;
    fd = open(FILENAME, O_RDONLY);
    if(fd >= 0)
    {
        // Clear the text and memory
        clrscr();
        memset((void*)MY_SCRN_MEM, 0x00, 0x3000);

        save_current_graphics_state();
        
        enable_console();
        
        gotoxy(0,0);
        cursor(1);

        console_update();
        /*
        do
        {
            input = cgetc();

            if(input == CH_ENTER)
            {
                console_state = !console_state;
                #ifdef GR_8
                set_graphics(GRAPHICS_8);
                #else
                set_graphics(GRAPHICS_9);
                #endif
            }
            else
            {
                cputc(input);
            }

        } while(input != CH_ESC);
        */

        restore_graphics_state();
    }
    else
    {
        printf("Unable to open the file %s\n", FILENAME);
    }
    
    // // Return the graphics modes
    // #ifdef GR_8
    // #else
    // POKE(NMIEN, NMI_STATE);      // Restore the previous NMI state
    // POKE(GPRIOR, ORG_GFX_STATE); // Restore the previous priority states
    // #endif
    // POKEW(SDLSTL, ORG_DLIST);    // Tell ANTIC to use the original display list
    #endif

    return 0;
}