// Copyright (C) 2021 Brad Colbert

#include "graphics.h"
#include "console.h"
#include "files.h"
#include "consts.h"
#include "types.h"
#include "utility.h"

#include <conio.h>
#include <atari.h>
#include <peekpoke.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

//
void printModeDefs(DLModeDef modeDefs[]);

// Globals
unsigned ORG_DLIST = 0;
unsigned VDSLIST_STATE = 0;
byte ORG_GPRIOR = 0x0;
byte NMI_STATE = 0x0;
byte ORG_COLOR1, ORG_COLOR2;
byte GRAPHICS_MODE = 0x0;

void saveCurrentGraphicsState(void)
{
    ORG_DLIST = PEEKW(SDLSTL);
    VDSLIST_STATE = PEEKW(VDSLST);
    NMI_STATE = ANTIC.nmien;// PEEK(NMIEN);
    ORG_GPRIOR = PEEK(GPRIOR);       // Save current priority states
    ORG_COLOR1 = PEEK(COLOR1);
    ORG_COLOR2 = PEEK(COLOR2);
    //CONSOLE_MEM = PEEKW(SAVMSC);
}

void restoreGraphicsState(void)
{
    POKE(NMIEN, NMI_STATE);
    POKEW(VDSLST, VDSLIST_STATE);
    POKEW(SDLSTL, ORG_DLIST);
    POKE(COLOR1, ORG_COLOR1);
    POKE(COLOR2, ORG_COLOR2);
    POKE(GPRIOR, ORG_GPRIOR);       // restore priority states
}

void makeDisplayList(byte mode, const MemSegs* buffInfo, DLDef* dlInfo)
{
    size_t memPerLine = 0;
    size_t linesPerSeg = 0;
    byte segCount = 0, modeCount = 0;
    byte* dlCmd = NULL;
    unsigned i = 0;

    switch(mode)
    {
        case GRAPHICS_0: // {0, DL_CHR40x8x1, 1, 0, CONSOLE_MEM}
        {
            DLModeDef def = {8, DL_CHR40x8x1, GFX_0_MEM_LINE, 0, 0};
            memPerLine = GFX_0_MEM_LINE;
            dlInfo->modes[0] = def;
            dlInfo->modes[1].blank_lines = 0xFF;
            break;
        }
        case GRAPHICS_8: // {8, DL_MAP320x1x1, 211, 0, 0}
        case GRAPHICS_9:
        case GRAPHICS_10:
        case GRAPHICS_11:
        {
            DLModeDef def = {8, DL_MAP320x1x1, GFX_8_LINES, 0, 0};
            memPerLine = GFX_8_MEM_LINE;
            dlInfo->modes[0] = def;
            dlInfo->modes[1].blank_lines = 0xFF;
        }
    }

    #ifdef DEBUG_GRAPHICS
    clrscr();
    printModeDefs(dlInfo->modes);
    cgetc();
    clrscr();
    #endif

    // Allocate 1K of memory.  Will shrink when done.
    dlInfo->address = malloc_constrianed(1024, 1024);
    dlCmd = dlInfo->address;

    #ifdef DEBUG_GRAPHICS
    cprintf("dlInfo_ad%02X dlCmd%02X\n\r", dlInfo->address, dlCmd);
    cgetc();
    clrscr();
    #endif

    while((dlInfo->modes[modeCount].blank_lines != 0xFF) && buffInfo->segs[segCount].size)
    {
        unsigned modeLineCount = 0, segLineCount = 0;
        linesPerSeg = buffInfo->segs[segCount].size / memPerLine;

        // Blank lines don't cost any buffer memory
        for(i = 0; i < dlInfo->modes[modeCount].blank_lines; ++i)
            *(dlCmd++) = (byte)DL_BLK8;

        for(; modeLineCount < dlInfo->modes[modeCount].lines; ++modeLineCount, ++segLineCount)
        {
            byte b = (byte)dlInfo->modes[modeCount].mode;

            if(segLineCount >= linesPerSeg)
            {
                ++segCount;
                linesPerSeg = buffInfo->segs[segCount].size / memPerLine; // need to generalize memPerLine. base from modeDef.
                segLineCount = 0;
            }

            if(segLineCount)
            {
                if(dlInfo->modes[modeCount].dli)
                    b = (byte)DL_DLI(b);  // DLI flag triggered so generate

                #ifdef DEBUG_GRAPHICS
                cprintf("%02X: %d %d %02X\n\r", ((unsigned)buffInfo->segs[segCount].addr)+(segLineCount*40), modeLineCount, segLineCount, b);
                #endif

                *(dlCmd++) = b;
            }
            else  // first line in segment
            {
                b = DL_LMS(b);

                if(dlInfo->modes[modeCount].dli)
                    b = DL_DLI(b);  // DLI flag triggered so generate

                *(dlCmd++) = b;

                #ifdef DEBUG_GRAPHICS
                cprintf("%02X: %d %d %02X ", ((unsigned)buffInfo->segs[segCount].addr)+(segLineCount*40), modeLineCount, segLineCount, b);
                #endif

                *((unsigned*)dlCmd) = (unsigned)buffInfo->segs[segCount].addr;  // Add the address

                #ifdef DEBUG_GRAPHICS
                cprintf("(%p)\n\r", buffInfo->segs[segCount].addr);
                #endif

                dlCmd+=2;
            }

            #ifdef DEBUG_GRAPHICS
            if(!((modeLineCount+1) % 23))
            {
                cgetc();
                clrscr();
            }
            #endif
        }

        ++modeCount;
    } // all modeDefs

    // Add the JVB
    *(dlCmd++) = DL_JVB;
    *((unsigned*)dlCmd) = (unsigned)dlInfo->address;
    dlCmd+=2;

    // Finally clean up and shrink the memory down to the size of the DL
    dlInfo->size = (size_t)(dlCmd - (byte*)dlInfo->address);
    dlInfo->address = realloc(dlInfo->address, dlInfo->size);
}

void makeGraphicsDef(byte mode, GfxDef* gfxInfo)
{
    memset(gfxInfo, 0, sizeof(GfxDef));

    switch(mode)
    {
        case GRAPHICS_0: // {0, DL_CHR40x8x1, 1, 0, CONSOLE_MEM}
            allocSegmentedMemory(GFX_0_MEM_LINE, GFX_0_LINES, 4096, &gfxInfo->buffer);
            break;
        case GRAPHICS_8: // {8, DL_MAP320x1x1, 211, 0, 0}
        case GRAPHICS_9:
        case GRAPHICS_10:
        case GRAPHICS_11:
            allocSegmentedMemory(GFX_8_MEM_LINE, GFX_8_LINES, 4096, &gfxInfo->buffer);
    }

    makeDisplayList(GRAPHICS_8, &gfxInfo->buffer, &gfxInfo->dl);
}

#ifdef DEBUG_GRAPHICS
void printModeDefs(DLModeDef modeDefs[])
{
    byte i = 0;
    for(; i < MAX_MODE_DEFS; ++i)
    {
        if(modeDefs[i].blank_lines == 0xFF)
            return;

        cprintf("%d, %d, %d, %d, %02X\n\r", modeDefs[i].blank_lines, modeDefs[i].mode, modeDefs[i].lines, modeDefs[i].dli, modeDefs[i].buffer);
    }
}

// Shows the contents of a display list.
// name - simply used in the output header so you can tell which DL is which on the console.
// mloc - the location of the DL in memory 
void printDList(const char* name, DLDef* dlInfo)
{
    byte b = 0x00, low, high;
    unsigned idx = 0;

    cprintf("Displaylist %s at %p (%d)\n\r", name, dlInfo->address, dlInfo->size);
    while(1)
    {
        if(idx)
            cprintf(", ");

        // Get the instruction
        b = ((byte*)dlInfo->address)[idx];
        if((b & 0x40) && (b & 0x0F)) // these have two address bytes following)
        {
            low = ((byte*)dlInfo->address)[++idx];
            high = ((byte*)dlInfo->address)[++idx];
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

#if 0
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
struct dl_store image_dl_store = { 0, 0 };

// Screen memory
void* MY_SCRN_MEM = 0x0;
void* MY_SCRN_MEM_TEMP = 0x0;// (MY_SCRN_MEM_C + 0x0400)

// Externals
extern byte console_state;

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
    //__asm__("sta %w", WSYNC);
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
    memset((void*)MY_SCRN_MEM, 0x00, 0x2280);
}

void set_graphics(byte mode)
{
    // Manage the frame buffer
    switch(mode)
    {
        case GRAPHICS_0:
            // Allocate the memory for the DL
            if(image_dl_store.mem)
                free(image_dl_store.mem);
            image_dl_store.mem = NULL;

            // Allocation the screen memory
            if(MY_SCRN_MEM)
                aligned_free(MY_SCRN_MEM);
            MY_SCRN_MEM = NULL;

            if(MY_SCRN_MEM_TEMP)
                aligned_free(MY_SCRN_MEM_TEMP);
            MY_SCRN_MEM_TEMP = NULL;
        break;

        default:
            // Allocate the memory for the DL
            if(image_dl_store.mem)
                free(image_dl_store.mem);
            image_dl_store.mem = malloc_constrianed(1024, 1024);

            // Allocation the screen memory
            if(MY_SCRN_MEM)
                aligned_free(MY_SCRN_MEM);
            MY_SCRN_MEM = aligned_malloc(40 * 220 + 36, 40); // 36 additional for waste due to 4K boundary

            if(MY_SCRN_MEM_TEMP)
                aligned_free(MY_SCRN_MEM_TEMP);
            MY_SCRN_MEM_TEMP = aligned_malloc(0x0400 + 16, 40); // Just in case we cross a 4K boundary

            cprintf("%p: %p %p\n\r", image_dl_store.mem, MY_SCRN_MEM, MY_SCRN_MEM_TEMP);
            cgetc();
        break;
    }

    // Turn off ANTIC while we muck with the DL
//    POKE(SDMCTL, 0);

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
            {
                dl_def command_dl_g8[] = { {8, DL_MAP320x1x1, 212, 0, 0},
                                           {0, DL_CHR40x8x1, 1, 0, CONSOLE_MEM}
                                         };
                command_dl_g8[0].address = (unsigned)MY_SCRN_MEM;

                POKE(GPRIOR, ORG_GPRIOR);     // Turn off GTIA
                POKE(NMIEN, NMI_STATE);       // Disable the NMI for DLIs'
                POKEW(VDSLST, VDSLIST_STATE); // Clear the DLI pointer
                makeDisplayList(image_dl_store.mem, command_dl_g8, 2, &image_dl_store);
// print_dlist("DLA", image_dl_store.mem);
// cgetc();
               POKE(COLOR2, 0);   // Background black
               POKE(COLOR1, 14);  // Color maximum luminance
            }
            break;
            case GRAPHICS_9:
            {
                dl_def command_dl_g9[] = { {8, DL_MAP320x1x1, 211, 0, 0},
                                           {0, DL_MAP320x1x1, 1, 1, 0x0},
                                           {0, DL_CHR40x8x1, 1, 1, CONSOLE_MEM}
                                         };
                command_dl_g9[0].address = (unsigned)MY_SCRN_MEM;

                makeDisplayList(image_dl_store.mem, command_dl_g9, 3, &image_dl_store);
// print_dlist("DLB", image_dl_store.mem);
// cgetc();
                POKE(COLOR2, 0);                        // Turn the console black
                POKE(GPRIOR, ORG_GPRIOR | GFX_9);       // Enable GTIA   
                POKEW(VDSLST, (unsigned)disable_9_dli); // Set the address to our DLI that disables GTIA for the console
                POKE(NMIEN, NMI_STATE | 192);           // Enable NMI
            }
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
            default:
            {
                dl_def image_dl[] = { {8, DL_MAP320x1x1, 220, 0, 0}
                                    };
                image_dl[0].address = (unsigned)MY_SCRN_MEM;

                makeDisplayList(image_dl_store.mem, image_dl, 1, &image_dl_store);
// print_dlist("DLC", image_dl_store.mem);
// cgetc();
            }
            break;
        }

        // Set graphics mode specifc things
        switch(mode)
        {
            case GRAPHICS_8:
                POKE(COLOR2, 0);   // Background black
                POKE(COLOR1, 14);  // Color maximum luminance
                POKE(GPRIOR, ORG_GPRIOR);     // Turn off GTIA
            //break;
            case GRAPHICS_9:
                POKE(GPRIOR, ORG_GPRIOR | GFX_9);   // Enable GTIA   
            break;
            case GRAPHICS_10:
                POKE(GPRIOR, ORG_GPRIOR | GFX_10);   // Enable GTIA   
            break;
            case GRAPHICS_11:
                POKE(GPRIOR, ORG_GPRIOR | GFX_11);   // Enable GTIA   
            break;
        }
    }

    switch(mode)
    {
        case GRAPHICS_0:
        break;
        // default:
        //     POKEW(SDLSTL, (unsigned)image_dl_store.mem);            // Tell ANTIC the address of our display list (use it)
    }

    // Turn ANTIC back on
//    POKE(SDMCTL, 0x22);

    GRAPHICS_MODE = mode;
}

void set_graphics_console(byte enable)
{
    console_state = enable;
    set_graphics(GRAPHICS_MODE);
}
#endif