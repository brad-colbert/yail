// Copyright (C) 2021 Brad Colbert
#include "graphics.h"
#include "console.h"
#include "files.h"
#include "utility.h"
#include "settings.h"
#include "consts.h"
#include "types.h"

#include <atari.h>
#include <peekpoke.h>
#ifdef DEBUG_GRAPHICS
#include <conio.h>
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#pragma bss-name (push,"FRAMEBUFFER")
byte framebuffer[FRAMEBUFFER_SIZE + 32];  // 32 bytes of padding
#pragma bss-name (pop)
#pragma optimize(push, off)
#pragma data-name (push,"GFX8_DL")
#include "graphics_8_dl.h"
#pragma data-name (pop)
#pragma data-name (push,"GFX8_CONSOLE_DL")
#include "graphics_8_console_dl.h"
#pragma data-name (pop)
#pragma data-name (push,"GFX9_CONSOLE_DL")
#include "graphics_9_console_dl.h"
#pragma data-name (pop)
#pragma optimize(pop)

#define FRAMEBUFFER_BlOCK_SIZE 0x1000
#define IS_LMS(x) (x & (byte)64)

//
extern char* console_buff;
extern byte console_lines;
extern Settings settings;

// Globals
void* ORG_SDLIST = 0;
void* VDSLIST_STATE = 0;
byte ORG_GPRIOR = 0x0;
byte NMI_STATE = 0x0;
byte ORG_COLOR1, ORG_COLOR2;
DLDef dlDef;
ImageData image = { {0, 0, 0, 0, 0, 0}, framebuffer };

// DLI definitions
void disable_9_dli(void);  // prototype for below

// Enable Gfx 9
#pragma optimize(push, off)
void enable_9_dli(void) {
    __asm__("pha");
    __asm__("tya");
    __asm__("pha");
    __asm__("txa");
    __asm__("pha");
    __asm__("sta %w", WSYNC);
    POKE(PRIOR, ORG_GPRIOR | GFX_9);
    POKEW(VDSLST, (unsigned)disable_9_dli);
    __asm__("pla");
    __asm__("tax");
    __asm__("pla");
    __asm__("tay");
    __asm__("pla");
    __asm__("rti");
}

// Disable Gfx 9
void disable_9_dli(void) {
    __asm__("pha");
    __asm__("tya");
    __asm__("pha");
    __asm__("txa");
    __asm__("pha");
    __asm__("sta %w", WSYNC);
    POKE(PRIOR, ORG_GPRIOR);
    POKEW(VDSLST, (unsigned)enable_9_dli);
    __asm__("pla");
    __asm__("tax");
    __asm__("pla");
    __asm__("tay");
    __asm__("pla");
    __asm__("rti");
}
#pragma optimize(pop)

void saveCurrentGraphicsState(void)
{
    ORG_SDLIST = OS.sdlst;
    VDSLIST_STATE = OS.vdslst;
    NMI_STATE = ANTIC.nmien;
    ORG_GPRIOR = OS.gprior;       // Save current priority states
    ORG_COLOR1 = OS.color1;
    ORG_COLOR2 = OS.color2;
}

void restoreGraphicsState(void)
{
    ANTIC.nmien = NMI_STATE;
    OS.vdslst = VDSLIST_STATE;
    OS.sdlst = ORG_SDLIST;
    OS.color1 = ORG_COLOR1;
    OS.color2 = ORG_COLOR2;
    OS.gprior = ORG_GPRIOR;       // restore priority states
    OS.botscr = 24;
}

void setGraphicsMode(const byte mode)
{
    if(mode == settings.gfx_mode)
        return;

    makeDisplayList(mode);

    OS.color1 = 14;         // Color maximum luminance
    OS.color2 = 0;          // Background black

    switch(mode)
    {
        case GRAPHICS_0:
            OS.sdlst = ORG_SDLIST;
            ANTIC.nmien = NMI_STATE;
            OS.vdslst = VDSLIST_STATE;
            OS.botscr = 24;
        break;

        case GRAPHICS_8:
        case GRAPHICS_9:
        case GRAPHICS_10:
        case GRAPHICS_11:
            OS.sdlst = &graphics_8_dl;
        break;

        case GRAPHICS_8_CONSOLE:
            OS.sdlst = &graphics_8_console_dl;
        break;
        case GRAPHICS_9_CONSOLE:
        case GRAPHICS_10_CONSOLE:
        case GRAPHICS_11_CONSOLE:
            OS.sdlst = &graphics_9_console_dl;
        break;
    }

    // Set graphics mode specifc things
    switch(mode & ~GRAPHICS_CONSOLE_EN)
    {
        case GRAPHICS_0:
        case GRAPHICS_8:
            OS.gprior = ORG_GPRIOR;           // Return original state of GTIA
        break;
        case GRAPHICS_9:
            OS.gprior = ORG_GPRIOR | GFX_9;   // Enable GTIA   
        break;
        case GRAPHICS_10:
            OS.gprior = ORG_GPRIOR | GFX_10;   // Enable GTIA   
        break;
        case GRAPHICS_11:
            OS.gprior = ORG_GPRIOR | GFX_11;   // Enable GTIA   
        break;
    }

    settings.gfx_mode = mode;

    put_settings(SETTINGS_GFX);  // save the graphics mode on the FN
}

void makeDisplayList(byte mode)
{
    switch(mode)
    {
        case GRAPHICS_0: // {0, DL_CHR40x8x1, 1, 0, CONSOLE_MEM}
            OS.sdlst = ORG_SDLIST;
        break;
        case GRAPHICS_8: // {8, DL_MAP320x1x1, 211, 0, 0}
        case GRAPHICS_9:
        case GRAPHICS_10:
        case GRAPHICS_11:
            dlDef.address = &graphics_8_dl;
        break;
        case GRAPHICS_8_CONSOLE: // {8, DL_MAP320x1x1, 211, 0, 0}
            dlDef.address = &graphics_8_console_dl;
        break;
        case GRAPHICS_9_CONSOLE:
        case GRAPHICS_10_CONSOLE:
        case GRAPHICS_11_CONSOLE:
            dlDef.address = &graphics_9_console_dl;
        break;
    } // switch mode
}

void show_console()
{
    switch(settings.gfx_mode)
    {
        case GRAPHICS_0: // {0, DL_CHR40x8x1, 1, 0, CONSOLE_MEM}
            break;
        case GRAPHICS_8: // {8, DL_MAP320x1x1, 211, 0, 0}
        {
            settings.gfx_mode |= GRAPHICS_CONSOLE_EN;

            makeDisplayList(settings.gfx_mode);

            OS.sdlst = dlDef.address;
            ANTIC.nmien = 0x40;
            OS.botscr = 5;
        }
        break;
        case GRAPHICS_9:
        case GRAPHICS_10:
        case GRAPHICS_11:
        {
            settings.gfx_mode |= GRAPHICS_CONSOLE_EN;

            makeDisplayList(settings.gfx_mode);

            OS.sdlst = dlDef.address;
            OS.vdslst = disable_9_dli;
            ANTIC.nmien = 0x80 | 0x40;
            OS.botscr = 5;
        }
    }
}

void hide_console()
{
    switch(settings.gfx_mode) // ^ GRAPHICS_CONSOLE_EN)
    {
        case GRAPHICS_0: // {0, DL_CHR40x8x1, 1, 0, CONSOLE_MEM}
            break;
        case GRAPHICS_8_CONSOLE: // {8, DL_MAP320x1x1, 211, 0, 0}
        case GRAPHICS_9_CONSOLE:
        case GRAPHICS_10_CONSOLE:
        case GRAPHICS_11_CONSOLE:
        {
            settings.gfx_mode &= (byte)~GRAPHICS_CONSOLE_EN;

            makeDisplayList(settings.gfx_mode);
            //POKEW(SDLSTL, gfxState.dl.address);

            OS.sdlst = dlDef.address;
            ANTIC.nmien = 0x40;
            OS.vdslst = VDSLIST_STATE;

            OS.botscr = 0;
        }
    }
}

void clearFrameBuffer(void)
{
    memset(framebuffer, 0, FRAMEBUFFER_SIZE + 32);
}

// Shows the contents of a display list.
// name - simply used in the output header so you can tell which DL is which on the console.
// mloc - the location of the DL in memory
#ifdef DEBUG_GRAPHICS
void printDList(const char* name)
{
    byte b = 0x00, low, high;
    unsigned idx = 0;

    cprintf("Displaylist %s at %p (%d)\n\r", name, dlDef.address, dlDef.size);
    while(1)
    {
        if(idx)
            cprintf(", ");

        // Get the instruction
        b = ((byte*)dlDef.address)[idx];
        if((b & 0x40) && (b & 0x0F)) // these have two address bytes following)
        {
            low = ((byte*)dlDef.address)[++idx];
            high = ((byte*)dlDef.address)[++idx];
            cprintf("%02X (%02x%02x)", b, low, high);
            cgetc();
            clrscr();
        }
        else
            cprintf("%02X", b);

        idx++;

        if(b == 0x41) // JVB so done... maybe  have to add code to look at the address
            break;
    }
}
#endif
