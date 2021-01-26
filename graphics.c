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

void printModeDefs(DLModeDefParray modeDefs)
{
    byte count = 0;
    DLModeDef* modeDef = modeDefs[count];
    while(modeDef)
    {
        cprintf("%d, %d, %d, %d, %02X\n\r", modeDef->blank_lines, modeDef->mode, modeDef->lines, modeDef->dli, modeDef->buffer);
        modeDef = modeDefs[++count];
    }
}

void makeDisplayList(byte mode, const MemSegs* buffInfo, DLDef* dlInfo)
{
    // Allocate the buffer memory
    size_t memPerLine = 0;
    size_t linesPerSeg = 0;
    DLModeDefParray modeDefs = NULL;
    DLModeDef* modeDef = NULL;
    MemSeg* segDef = NULL;
    byte segCount = 0, modeCount = 0;
    byte* dlCmd = NULL;
    unsigned i = 0;
    segDef = buffInfo->segs[0];

    printMemorySegmentationTable(buffInfo);
    cputs("----------------------\n\r");
    cgetc();

    switch(mode)
    {
        case GRAPHICS_0: // {0, DL_CHR40x8x1, 1, 0, CONSOLE_MEM}
            memPerLine = GFX_0_MEM_LINE;
            // modeDefs = calloc(2, sizeof(DLModeDef*));
            // modeDefs[0] = calloc(1, sizeof(DLModeDef));
            modeDefs = malloc(sizeof(DLModeDef*));
            modeDefs[0] = malloc(sizeof(DLModeDef) * 2);
            modeDefs[0]->blank_lines = 1;
            modeDefs[0]->mode = DL_CHR40x8x1;
            modeDefs[0]->lines = GFX_0_LINES;
            modeDefs[1] = NULL;
            break;
        case GRAPHICS_8: // {8, DL_MAP320x1x1, 211, 0, 0}
        case GRAPHICS_9:
        case GRAPHICS_10:
        case GRAPHICS_11:
            memPerLine = GFX_8_MEM_LINE;
            // modeDefs = calloc(2, sizeof(DLModeDef*));
            // modeDefs[0] = calloc(1, sizeof(DLModeDef));
            modeDefs = malloc(sizeof(DLModeDef*));
            modeDefs[0] = malloc(sizeof(DLModeDef) * 2);
            modeDefs[0]->blank_lines = 8;
            modeDefs[0]->mode = DL_MAP320x1x1;
            modeDefs[0]->lines = GFX_8_LINES;
            modeDefs[1] = NULL;
    }
    cprintf("%p: %p %p\n\r", modeDefs, modeDefs[0], modeDefs[1]);
    cputs("----------------------\n\r");
    cgetc();

    printMemorySegmentationTable(buffInfo);
    cgetc();

    printModeDefs(modeDefs);
    cgetc();


    // Allocate 1K of memory.  Will shrink when done.
    dlInfo->address = malloc_constrianed(1024, 1024);
    dlCmd = dlInfo->address;

    cprintf("dlInfo_ad%02X dlCmd%02X\n\r", dlInfo->address, dlCmd);
    cgetc();

    modeDef = modeDefs[modeCount];
    while(modeDef && segDef)
    {
        unsigned modeLineCount = 0, segLineCount = 0;
        linesPerSeg = segDef->size / memPerLine;

        // Blank lines don't cost any buffer memory
        for(i = 0; i < modeDef->blank_lines; ++i)
            *(dlCmd++) = (byte)DL_BLK8;

        for(; modeLineCount < modeDef->lines; ++modeLineCount, ++segLineCount)
        {
            byte b = (byte)modeDef->mode;

            if(segLineCount >= linesPerSeg)
            {
                ++segCount;
                segDef = buffInfo->segs[segCount];
                linesPerSeg = segDef->size / memPerLine; // need to generalize memPerLine. base from modeDef.
                segLineCount = 0;
            }

            if(segLineCount)
            {
                if(modeDef->dli)
                    b = (byte)DL_DLI(b);  // DLI flag triggered so generate

                cprintf("%02X: %d %d %02X\n\r", ((unsigned)segDef->addr)+(segLineCount*40), modeLineCount, segLineCount, b);

                *(dlCmd++) = b;
            }
            else  // first line in segment
            {
                b = DL_LMS(b);

                if(modeDef->dli)
                    b = DL_DLI(b);  // DLI flag triggered so generate

                *(dlCmd++) = b;

                cprintf("%02X: %d %d %02X ", ((unsigned)segDef->addr)+(segLineCount*40), modeLineCount, segLineCount, b);

                *((unsigned*)dlCmd) = (unsigned)segDef->addr;  // Add the address

                cprintf("(%p)\n\r", segDef->addr);

                dlCmd+=2;
            }

            if(!(modeLineCount % 20))
            {
                cgetc();
                clrscr();
            }
        }
    } // all modeDefs

    // Add the JVB
    *(dlCmd++) = DL_JVB;
    *((unsigned*)dlCmd) = (unsigned)dlInfo->address;
    dlCmd+=2;

    // Finally clean up and shrink the memory down to the size of the DL
    dlInfo->size = (size_t)(dlCmd - (byte*)dlInfo->address);
    dlInfo->address = realloc(dlInfo->address, dlInfo->size);
    dlInfo->modes = modeDefs;
}

// A little trickery, first we malloc a byte the size + some fudge.  Assuming it fits
// free it and malloc the real deal;
void allocateBuffer(size_t len, size_t alignment, MemSegs* segs)
{
    size_t n = len / 0x1000;  // 4k chunks, how many do we cross? not perfect
    void* test = malloc(len + (16 * n));
    computeMemorySegmentation(test, len, alignment, 0x1000, segs);
    // Now reallocate the memory using which should be in the same place with the right size.
    test = realloc(test, segs->size);
    ((byte*)test)[segs->size-2] = 0xBE;
    ((byte*)test)[segs->size-1] = 0xEF;

    cprintf("%p X %p\n\r", test, segs->segs[0]->addr);
    cgetc();
}

void makeGraphicsDef(byte mode, GfxDef* gfxInfo)
{
    size_t buffSize = 0;
    size_t alignment;

    // Allocate the buffer memory
    switch(mode & 0x0F)
    {
        case GRAPHICS_0:
            buffSize = GFX_0_MEM_LINE * GFX_0_LINES;
            alignment = GFX_0_MEM_LINE;
            break;
        case GRAPHICS_8:
            buffSize = GFX_8_MEM_LINE * GFX_8_LINES;
            alignment = GFX_8_MEM_LINE;
            break;
        case GRAPHICS_9:
            buffSize = GFX_9_MEM_LINE * GFX_9_LINES;
            alignment = GFX_9_MEM_LINE;
            break;
        case GRAPHICS_10:
            buffSize = GFX_10_MEM_LINE * GFX_10_LINES;
            alignment = GFX_10_MEM_LINE;
            break;
        case GRAPHICS_11:
            buffSize = GFX_11_MEM_LINE * GFX_11_LINES;
            alignment = GFX_11_MEM_LINE;
            break;
    }

    allocateBuffer(buffSize, alignment, &gfxInfo->buffer);

    printMemorySegmentationTable(&gfxInfo->buffer);
    cgetc();

    //clrscr();

    // Make the display list
    makeDisplayList(mode, &gfxInfo->buffer, &gfxInfo->dl);
    cgetc();
    clrscr();
    printDList("DL", &gfxInfo->dl);
    cgetc();
}

// Shows the contents of a display list.
// name - simply used in the output header so you can tell which DL is which on the console.
// mloc - the location of the DL in memory 
void printDList(const char* name, DLDef* dlInfo)
{
    byte b = 0x00, low, high;
    unsigned idx = 0;

    cprintf("Displaylist %s at %p (%d)\n", name, dlInfo->address, dlInfo->size);
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
        }
        else
            cprintf("%02X", b);

        idx++;

        if(b == 0x41) // JVB so done... maybe  have to add code to look at the address
            break;
    }
}
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