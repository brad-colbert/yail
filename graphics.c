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

//
extern char* console_buff;
extern byte console_lines;

// Globals
void* ORG_SDLIST = 0;
void* VDSLIST_STATE = 0;
byte ORG_GPRIOR = 0x0;
byte NMI_STATE = 0x0;
byte ORG_COLOR1, ORG_COLOR2;
GfxDef gfxState;

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
    ORG_SDLIST = OS.sdlst; //PEEKW(SDLSTL);
    VDSLIST_STATE = OS.vdslst; //PEEKW(VDSLST);
    NMI_STATE = ANTIC.nmien;
    ORG_GPRIOR = OS.gprior;       // Save current priority states
    ORG_COLOR1 = OS.color1;
    ORG_COLOR2 = OS.color2;
    //CONSOLE_MEM = PEEKW(SAVMSC);
}

void restoreGraphicsState(void)
{
    ANTIC.nmien = NMI_STATE; //POKE(NMIEN, NMI_STATE);
    OS.vdslst = VDSLIST_STATE; //POKEW(VDSLST, VDSLIST_STATE);
    OS.sdlst = ORG_SDLIST; // POKEW(SDLSTL, ORG_SDLIST);
    OS.color1 = ORG_COLOR1; //POKE(COLOR1, ORG_COLOR1);
    OS.color2 = ORG_COLOR2; //POKE(COLOR2, ORG_COLOR2);
    OS.gprior = ORG_GPRIOR; //POKE(GPRIOR, ORG_GPRIOR);       // restore priority states
}

#define IS_LMS(x) (x & 64)

void generateDisplayList(const MemSegs* buffInfo, DLDef* dlInfo)
{
    byte segCount = 0;
    MemSeg* segDef = NULL;
    byte modeCount = 0;
    DLModeDef* modeDef = &dlInfo->modes[0];
    byte* dlCmd = NULL;
    unsigned line_in_seg = 1;

    // Allocate 1K of memory.  Will shrink when done.
    //clrscr();
    //cprintf("dl %p\n\r", gfxState.dl.address);
    if(!dlInfo->address)
        dlInfo->address = malloc_constrianed(1024, 1024);
    // cprintf("dl %p\n\r", gfxState.dl.address);
    // cgetc();
    dlCmd = dlInfo->address;

    #ifdef DEBUG_GRAPHICS
    cprintf("dlInfo_ad%02X dlCmd%02X\n\r", dlInfo->address, dlCmd);
    cgetc();
    clrscr();
    #endif

    while(modeDef->blank_lines != 0xFF) // for all of the modelines
    {
        int i = 0;
        unsigned line;
        void* lms_addr = NULL;

        // Convert the modelines to DL instructions.

        // Blank spaces
        for(; i < modeDef->blank_lines/8; ++i)
            *(dlCmd++) = (byte)DL_BLK8;

        // Iterate through all of the lines
        for(line = 0; line < modeDef->lines; ++line)
        {
            byte b = (byte)modeDef->mode;
            size_t size_block = 0;
            
            segDef = &((MemSegs*)buffInfo)->segs[segCount];
            size_block = (segDef->size / segDef->block_size) * segDef->block_size;

            if(!line) // first line
            {
                if(modeDef->buffer) // references a buffer address so LMS
                {
                    b = DL_LMS(b);
                    lms_addr = modeDef->buffer;
                }
            }

            if((line_in_seg * GFX_8_MEM_LINE) > size_block)
            {
                #ifdef DEBUG_GRAPHICS
                cprintf("%d:%d %d:%d %d %d %p ", modeCount, line, segCount, line_in_seg, line_in_seg * GFX_8_MEM_LINE, size_block, segDef->addr);
                #endif

                // Crossing memory segments means that this is an LMS.
                b = DL_LMS(b);

                ++segCount;
                line_in_seg = 1;

                lms_addr = buffInfo->segs[segCount].addr;
                #ifdef DEBUG_GRAPHICS
                cprintf("%p   \n\r", lms_addr);
                #endif
            }
            else
            {
                #ifdef DEBUG_GRAPHICS
                cprintf("%d:%d %d:%d %d %d %p        \n\r", modeCount, line, segCount, line_in_seg, line_in_seg * GFX_8_MEM_LINE, size_block, segDef->addr);
                #endif
            }
            ++line_in_seg;

            #ifdef DEBUG_GRAPHICS
            if(!((line+1)%24))
            {
                cgetc();
                clrscr();
            }
            #endif

            if(modeDef->dli)
                b = DL_DLI(b);

            *(dlCmd++) = b;

            if(IS_LMS(b))
            {
                *((unsigned*)dlCmd) = lms_addr;  // Add the address
                dlCmd += 2;
            }
        }

        ++modeCount;
        modeDef = &dlInfo->modes[modeCount];
    }

    // Add the JVB
    *(dlCmd++) = DL_JVB;
    *((unsigned*)dlCmd) = (unsigned)dlInfo->address;
    dlCmd+=2;

    // Finally clean up and shrink the memory down to the size of the DL
    dlInfo->size = (size_t)(dlCmd - (byte*)dlInfo->address);

    #ifdef DEBUG_GRAPHICS
    {
        int i;
        for(i = 0; i < dlInfo->size; ++i)
        {
            if(i)
                cputs(",");
            cprintf("%02X", ((byte*)dlInfo->address)[i]);
        }
    }
    #endif


    //dlInfo->address = realloc(dlInfo->address, dlInfo->size);
}

void makeDisplayList(byte mode, const MemSegs* buffInfo, DLDef* dlInfo)
{
    size_t memPerLine = 0;
    size_t linesPerSeg = 0;
    byte segCount = 0, modeCount = 0;
    byte* dlCmd = NULL;
    unsigned i = 0;

    // Clear the modes
    memset(&dlInfo->modes, 0, sizeof(DLModeDef[MAX_MODE_DEFS]));

    switch(mode)
    {
        case GRAPHICS_0: // {0, DL_CHR40x8x1, 1, 0, CONSOLE_MEM}
        {
            DLModeDef def = {16, DL_CHR40x8x1, GFX_0_LINES, 0, 0};
            def.buffer = buffInfo->segs[0].addr;
            memPerLine = GFX_0_MEM_LINE;
            dlInfo->modes[0] = def;
            dlInfo->modes[1].blank_lines = 0xFF;
        }
        break;
        case GRAPHICS_8: // {8, DL_MAP320x1x1, 211, 0, 0}
        case GRAPHICS_9:
        case GRAPHICS_10:
        case GRAPHICS_11:
        {
            DLModeDef def = {8, DL_MAP320x1x1, GFX_8_LINES, 0, 0};
            def.buffer = buffInfo->segs[0].addr;
            memPerLine = GFX_8_MEM_LINE;
            dlInfo->modes[0] = def;
            dlInfo->modes[1].blank_lines = 0xFF;
        }
        break;
        case GRAPHICS_8_CONSOLE: // {8, DL_MAP320x1x1, 211, 0, 0}
        {
            DLModeDef gfx = {8, DL_MAP320x1x1, 0, 0, 0x0};
            DLModeDef console = {0, DL_CHR40x8x1, 0, 0, 0x0};
            gfx.lines = GFX_8_LINES - (8 * console_lines);
            gfx.buffer = buffInfo->segs[0].addr;
            console.lines = console_lines;
            console.buffer = OS.savmsc; //console_buff;
            memPerLine = GFX_8_MEM_LINE;
            dlInfo->modes[0] = gfx;
            dlInfo->modes[1] = console;
            dlInfo->modes[2].blank_lines = 0xFF;
        }
        break;
        case GRAPHICS_9_CONSOLE:
        case GRAPHICS_10_CONSOLE:
        case GRAPHICS_11_CONSOLE:
        {
            DLModeDef gfx = {8, DL_MAP320x1x1, 0, 0, 0x0};
            DLModeDef gfx_dli = {0, DL_MAP320x1x1, 1, 1, 0x0};
            gfx.lines = (GFX_8_LINES - (8 * console_lines)) - 1;
            gfx.buffer = buffInfo->segs[0].addr;
            memPerLine = GFX_8_MEM_LINE;
            dlInfo->modes[0] = gfx;
            dlInfo->modes[1] = gfx_dli;
            if(console_lines > 1)
            {
                DLModeDef console = {0, DL_CHR40x8x1, 0, 0, 0x0};
                DLModeDef console_dli = {0, DL_CHR40x8x1, 1, 1, 0x0};
                console.lines = console_lines - 1;
                console.buffer = OS.savmsc; //console_buff;
                dlInfo->modes[2] = console;
                dlInfo->modes[3] = console_dli;
                dlInfo->modes[4].blank_lines = 0xFF;
            }
            else
            {
                DLModeDef console = {0, DL_CHR40x8x1, 1, 1, 0x0};
                console.buffer = OS.savmsc; //console_buff;
                dlInfo->modes[2] = console;
                dlInfo->modes[3].blank_lines = 0xFF;
            }
        }
        break;

    } // switch mode

    #ifdef DEBUG_GRAPHICS
    clrscr();
    printModeDefs(dlInfo->modes);
    cgetc();
    clrscr();
    #endif

    generateDisplayList(buffInfo, dlInfo);
}

void freeDisplayList(DLDef* dlInfo)
{
    free(dlInfo->address);
    memset(dlInfo, 0, sizeof(DLDef));
}

void makeGraphicsDef(byte mode, GfxDef* gfxInfo)
{
    switch(mode)
    {
        case GRAPHICS_0:
            allocSegmentedMemory(GFX_0_MEM_LINE, GFX_0_LINES, 4096, &gfxInfo->buffer);
            break;
        case GRAPHICS_8:
        case GRAPHICS_9:
        case GRAPHICS_10:
        case GRAPHICS_11:
        case GRAPHICS_8_CONSOLE:
        case GRAPHICS_9_CONSOLE:
            allocSegmentedMemory(GFX_8_MEM_LINE, GFX_8_LINES, 4096, &gfxInfo->buffer);
    }

    makeDisplayList(mode, &gfxInfo->buffer, &gfxInfo->dl);
}

void enableConsole()
{
    switch(gfxState.mode)
    {
        case GRAPHICS_0: // {0, DL_CHR40x8x1, 1, 0, CONSOLE_MEM}
            break;
        case GRAPHICS_8: // {8, DL_MAP320x1x1, 211, 0, 0}
        {
            gfxState.mode |= GRAPHICS_CONSOLE_EN;

            makeDisplayList(gfxState.mode, &gfxState.buffer, &gfxState.dl);
            //OS.sdlst = gfxState.dl;
            POKEW(SDLSTL, gfxState.dl.address);
            POKE(0xD40E, 0x40);

            //freeDisplayList(&gfxState.dl);
            //gfxState.dl = newDl;
        }
        break;
        case GRAPHICS_9:
        case GRAPHICS_10:
        case GRAPHICS_11:
        {
            gfxState.mode |= GRAPHICS_CONSOLE_EN;

            makeDisplayList(gfxState.mode, &gfxState.buffer, &gfxState.dl);
            //OS.sdlst = gfxState.dl;
            POKEW(SDLSTL, gfxState.dl.address);

            //freeDisplayList(&gfxState.dl);
            //gfxState.dl = newDl;

            //OS.vdslst = disable_9_dli;
            //ANTIC.nmien = 0x80 | 0x40;
            POKEW(VDSLST, disable_9_dli);
            POKE(0xD40E, 0x80 | 0x40);
        }

    }
}

void disableConsole()
{
    switch(gfxState.mode ^ GRAPHICS_CONSOLE_EN)
    {
        case GRAPHICS_0: // {0, DL_CHR40x8x1, 1, 0, CONSOLE_MEM}
            break;
        case GRAPHICS_8: // {8, DL_MAP320x1x1, 211, 0, 0}
        case GRAPHICS_9:
        case GRAPHICS_10:
        case GRAPHICS_11:
        {
            gfxState.mode &= ~GRAPHICS_CONSOLE_EN;

            makeDisplayList(gfxState.mode, &gfxState.buffer, &gfxState.dl);
            //OS.sdlst = newDl.address;
            POKEW(SDLSTL, gfxState.dl.address);

            //freeDisplayList(&gfxState.dl);
            //gfxState.dl = newDl;

            //ANTIC.nmien = 0x40; //= NMI_STATE;
            POKE(0xD40E, 0x40);
            POKEW(VDSLST, VDSLIST_STATE);
            //OS.vdslst = VDSLIST_STATE;
        }
    }
}

void setGraphicsMode(byte mode, byte keep)
{
    if(mode == gfxState.mode)
        return;

    if(!gfxState.buffer.size)// !keep)
    {
        // free any current mode
        //freeDisplayList(&gfxState.dl);
        freeSegmentedMemory(&gfxState.buffer);

        // 
        makeGraphicsDef(mode, &gfxState);
    }

    #ifdef DEBUG_GRAPHICS
    cprintf("%p %p\n\r", gfxState.dl.address, gfxState.buffer.start);
    cgetc();
    #endif

    OS.color1 = 14;         // Color maximum luminance
    OS.color2 = 0;          // Background black

    switch(mode)
    {
        case GRAPHICS_0:
            OS.sdlst = gfxState.dl.address; //ORG_SDLIST;
            ANTIC.nmien = NMI_STATE;
            OS.vdslst = VDSLIST_STATE;
        break;
        default:
            OS.sdlst = gfxState.dl.address;
        break;
    }

    // Set graphics mode specifc things
    switch(mode & ~GRAPHICS_CONSOLE_EN)
    {
        case GRAPHICS_0:
        case GRAPHICS_8:
            OS.gprior = ORG_GPRIOR; // Turn off GTIA
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

    gfxState.mode = mode;
}

void clearFrameBuffer()
{
    memset(gfxState.buffer.start, 0, gfxState.buffer.size);
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