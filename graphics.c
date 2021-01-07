// Copyright (C) 2021 Brad Colbert

#include "graphics.h"
#include "console.h"
#include "displaylist.h"
#include "consts.h"
#include "types.h"

#include <conio.h>
#include <atari.h>
#include <peekpoke.h>
#include <string.h>
#include <stdio.h>

// Globals (private)
unsigned ORG_DLIST = 0;
unsigned VDSLIST_STATE = 0;
byte ORG_GPRIOR = 0x00;
byte NMI_STATE = 0x00;
byte WSYNC_STATE = 0x00;
byte ORG_COLOR1, ORG_COLOR2;
byte GRAPHICS_MODE = 0x00;
//unsigned CONSOLE_MEM = 0xFFFF;

// Display list definitions
struct dl_def image_dl[] = { {8, DL_MAP320x1x1, 102, MY_SCRN_MEM, 0},
                             {0, DL_MAP320x1x1, 102, MY_SCRN_MEM_B, 0},
                             {0, DL_MAP320x1x1, 16, MY_SCRN_MEM_C, 0}
                            };
struct dl_def command_dl_g8[] = { {8, DL_MAP320x1x1, 102, MY_SCRN_MEM, 0},
                                  {0, DL_MAP320x1x1, 102, MY_SCRN_MEM_B, 0},
                                  {0, DL_MAP320x1x1, 8, MY_SCRN_MEM_C, 0},
                                  {0, DL_CHR40x8x1, 1, CONSOLE_MEM, 0}
                                };
struct dl_def command_dl_g9[] = { {8, DL_MAP320x1x1, 102, MY_SCRN_MEM, 0},
                                  {0, DL_MAP320x1x1, 102, MY_SCRN_MEM_B, 0},
                                  {0, DL_MAP320x1x1, 7, MY_SCRN_MEM_C, 0},
                                  {0, DL_MAP320x1x1, 1, 0x0, 1},
                                  {0, DL_CHR40x8x1, 1, CONSOLE_MEM, 1}
                                };

// Externals
extern byte console_state;

// DLI definitions
void disable_9_dli(void);  // prototype for below

// Enable Gfx 9
#pragma optimize(push, off)
void enable_9_dli(void) {
    __asm__("sta %w", WSYNC);
    __asm__("pha");
    POKE(PRIOR, ORG_GPRIOR | GFX_9);
    POKEW(VDSLST, (unsigned)disable_9_dli);
    __asm__("pla");
    __asm__("rti");
}
#pragma optimize(pop)

// Disable Gfx 9
#pragma optimize(push, off)
void disable_9_dli(void) {
    __asm__("sta %w", WSYNC);
    __asm__("pha");
    POKE(PRIOR, ORG_GPRIOR);
    POKEW(VDSLST, (unsigned)enable_9_dli);
    __asm__("pla");
    __asm__("rti");
}
#pragma optimize(pop)

void save_current_graphics_state(void)
{
    ORG_DLIST = PEEKW(SDLSTL);
    VDSLIST_STATE = PEEKW(VDSLST);
    NMI_STATE = PEEK(NMIEN);
    ORG_GPRIOR = PEEK(GPRIOR);       // Save current priority states
    ORG_COLOR1 = PEEK(COLOR1);
    ORG_COLOR2 = PEEK(COLOR2);
    //CONSOLE_MEM = PEEKW(SAVMSC);
}

void restore_graphics_state(void)
{
    POKE(NMIEN, NMI_STATE);
    POKEW(VDSLST, VDSLIST_STATE);
    POKEW(SDLSTL, ORG_DLIST);
    POKE(COLOR1, ORG_COLOR1);
    POKE(COLOR2, ORG_COLOR2);
    POKE(GPRIOR, ORG_GPRIOR);       // restore priority states
}

void graphics_clear()
{
    memset((void*)MY_SCRN_MEM, 0x00, 0x3000);
}

void set_graphics(byte mode)
{
    if(console_state)
    {
        switch(mode)
        {
            case GRAPHICS_0:
                POKE(NMIEN, NMI_STATE);
                POKEW(VDSLST, VDSLIST_STATE);
                POKEW(SDLSTL, ORG_DLIST);
                POKE(GPRIOR, ORG_GPRIOR);       // restore priority states
            break;
            case GRAPHICS_8:
                POKE(GPRIOR, ORG_GPRIOR);     // Turn off GTIA
                POKE(NMIEN, NMI_STATE);       // Disable the NMI for DLIs'
                POKEW(VDSLST, VDSLIST_STATE); // Clear the DLI pointer
                makeDisplayList((void*)IML_DL, command_dl_g8, 4);
                POKE(COLOR2, 0);   // Background black
                POKE(COLOR1, 14);  // Color maximum luminance
            break;
            case GRAPHICS_9:
                makeDisplayList((void*)IML_DL, command_dl_g9, 5);
                POKE(COLOR2, 0);                        // Turn the console black
                POKE(GPRIOR, ORG_GPRIOR | GFX_9);       // Enable GTIA   
                POKEW(VDSLST, (unsigned)disable_9_dli); // Set the address to our DLI that disables GTIA for the console
                POKE(NMIEN, NMI_STATE | 192);           // Enable NMI
            break;
        }
    }
    else
    {
        POKE(NMIEN, NMI_STATE);       // Disable the NMI for DLIs'
        POKEW(VDSLST, VDSLIST_STATE); // Clear the DLI pointer

        // Build the display list
        switch(mode)
        {
            case GRAPHICS_0:
                POKE(NMIEN, NMI_STATE);
                POKEW(VDSLST, VDSLIST_STATE);
                POKEW(SDLSTL, ORG_DLIST);
                POKE(GPRIOR, ORG_GPRIOR);       // restore priority states
            break;
            case GRAPHICS_8:
                POKE(GPRIOR, ORG_GPRIOR);     // Turn off GTIA
            case GRAPHICS_9:
                makeDisplayList((void*)IML_DL, image_dl, 3);
            break;
        }

        // Set graphics mode specifc things
        switch(mode)
        {
            case GRAPHICS_8:
                POKE(COLOR2, 0);   // Background black
                POKE(COLOR1, 14);  // Color maximum luminance
            break;
            case GRAPHICS_9:
                POKE(GPRIOR, ORG_GPRIOR | GFX_9);   // Enable GTIA   
            break;
        }
    }

    switch(mode)
    {
        case GRAPHICS_8:
        case GRAPHICS_9:
            POKEW(SDLSTL, IML_DL);            // Tell ANTIC the address of our display list (use it)
    }

    GRAPHICS_MODE = mode;
}

void set_graphics_console(byte enable)
{
    console_state = enable;
    set_graphics(GRAPHICS_MODE);
}
