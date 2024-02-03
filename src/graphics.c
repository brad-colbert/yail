// Copyright (C) 2021 Brad Colbert
//#define DEBUG

#include "graphics.h"
#include "console.h"
#include "files.h"
#include "utility.h"
#include "consts.h"
#include "types.h"
#ifdef USE_PREDEF_DLS
#include "graphics_8_dl.h"
#include "graphics_8_console_dl.h"
#include "graphics_9_console_dl.h"
#endif

#include <conio.h>
#include <atari.h>
#include <peekpoke.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#pragma bss-name (push,"FRAMEBUFFER")
byte framebuffer[FRAMEBUFFER_SIZE + 32];  // 32 bytes of padding
#pragma bss-name (pop)
#ifndef USE_PREDEF_DLS
#pragma bss-name (push,"DISPLAYLIST")
byte displaylist[DISPLAYLIST_SIZE];
#pragma bss-name (pop)
#endif

#define FRAMEBUFFER_BlOCK_SIZE 0x1000
#define IS_LMS(x) (x & (byte)64)

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
#ifndef USE_ORIGINAL
byte CURRENT_MODE = 0;
DLDef dlDef;
//GfxDef gfxState;
#else
GfxDef gfxState;
#endif

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
    POKE(0x2BF, 24);
}

#ifndef USE_ORIGINAL
void setGraphicsMode(const byte mode)
{
    if(mode == CURRENT_MODE)
        return;

    #ifdef USE_PREDEF_DLS
    #else
    dlDef.address = displaylist;  // Always use the same display list memory
    #endif
    makeDisplayList(mode);

    #ifdef DEBUG_GRAPHICS
    cprintf("%p %p\n\r", gfxState.dl.address, gfxState.buffer.start);
    cgetc();
    #endif

    OS.color1 = 14;         // Color maximum luminance
    OS.color2 = 0;          // Background black

    switch(mode)
    {
        case GRAPHICS_0:
            OS.sdlst = ORG_SDLIST;
            ANTIC.nmien = NMI_STATE;
            OS.vdslst = VDSLIST_STATE;
            POKE(0x2BF, 26);
        break;
        default:
            OS.sdlst = displaylist;
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

    CURRENT_MODE = mode;
}

void makeDisplayList(byte mode)
{
    #ifdef USE_PREDEF_DLS
    switch(mode)
    {
        case GRAPHICS_0: // {0, DL_CHR40x8x1, 1, 0, CONSOLE_MEM}
            OS.sdlst = ORG_SDLIST;
        break;
        case GRAPHICS_8: // {8, DL_MAP320x1x1, 211, 0, 0}
        case GRAPHICS_9:
        case GRAPHICS_10:
        case GRAPHICS_11:
            dlDef.address = graphics_8_dl;
        break;
        case GRAPHICS_8_CONSOLE: // {8, DL_MAP320x1x1, 211, 0, 0}
            dlDef.address = graphics_8_console_dl;
        break;
        case GRAPHICS_9_CONSOLE:
        case GRAPHICS_10_CONSOLE:
        case GRAPHICS_11_CONSOLE:
            dlDef.address = graphics_9_console_dl;
        break;
    } // switch mode    
    #else
    size_t linesPerSeg = 0;
    byte segCount = 0, modeCount = 0;
    byte* dlCmd = NULL;
    unsigned i = 0;

    // Clear the modes
    memset(dlDef.modes, 0, sizeof(dlDef.modes));

    switch(mode)
    {
        case GRAPHICS_0: // {0, DL_CHR40x8x1, 1, 0, CONSOLE_MEM}
        {
            DLModeDef def = {16, DL_CHR40x8x1, GFX_0_LINES, 0, framebuffer};
            dlDef.modes[0] = def;
            dlDef.modes[1].blank_lines = 0xFF;
        }
        break;
        case GRAPHICS_8: // {8, DL_MAP320x1x1, 211, 0, 0}
        case GRAPHICS_9:
        case GRAPHICS_10:
        case GRAPHICS_11:
        {
            DLModeDef def = {8, DL_MAP320x1x1, GFX_8_LINES, 0, framebuffer};
            dlDef.modes[0] = def;
            dlDef.modes[1].blank_lines = 0xFF;
        }
        break;
        case GRAPHICS_8_CONSOLE: // {8, DL_MAP320x1x1, 211, 0, 0}
        {
            DLModeDef gfx = {8, DL_MAP320x1x1, 0, 0, framebuffer};
            DLModeDef console = {0, DL_CHR40x8x1, 0, 0, 0x0};
            gfx.lines = GFX_8_LINES - (8 * console_lines);
            console.lines = console_lines;
            console.buffer = OS.savmsc; //console_buff;
            dlDef.modes[0] = gfx;
            dlDef.modes[1] = console;
            dlDef.modes[2].blank_lines = 0xFF;
        }
        break;
        case GRAPHICS_9_CONSOLE:
        case GRAPHICS_10_CONSOLE:
        case GRAPHICS_11_CONSOLE:
        {
            DLModeDef gfx = {8, DL_MAP320x1x1, 0, 0, framebuffer};
            DLModeDef gfx_dli = {0, DL_MAP320x1x1, 1, 1, 0x0};      // Turn off the GTIA gfx modes using a DLI
            DLModeDef gfx_std = {0, DL_MAP320x1x1, 1, 0, 0x0};      // Turn off the GTIA gfx modes using a DLI
            gfx.lines = GFX_8_LINES - (8 * console_lines) - 2;
            dlDef.modes[0] = gfx;
            dlDef.modes[1] = gfx_dli;
            dlDef.modes[2] = gfx_std;
            if(console_lines > 1)
            {
                DLModeDef console = {0, DL_CHR40x8x1, 0, 0, 0x0};
                DLModeDef console_dli = {0, DL_CHR40x8x1, 1, 1, 0x0};
                console.lines = console_lines-1;
                console.buffer = OS.savmsc;
                dlDef.modes[3] = console;
                dlDef.modes[4] = console_dli;
                dlDef.modes[5].blank_lines = 0xFF;
            }
            else
            {
                DLModeDef console = {0, DL_CHR40x8x1, 1, 1, 0x0};
                console.buffer = OS.savmsc;
                dlDef.modes[3] = console;
                dlDef.modes[4].blank_lines = 0xFF;
            }
        }
        break;

    } // switch mode

    generateDisplayList();
    #endif
}

void generateDisplayList(void)
{
    //#define DEBUG
    #ifdef USE_PREDEF_DLS
    #else
    // Display lists addressing are only able to address 4K of memory so they can't cross
    // 4K boundaries.  This means that we need to create a display list that addresses
    // each 4K block by inserting LMS instructions as needed.
    //const ushort MEM_PER_BLOCK = 0x1000;
    const ushort LINES_PER_BLOCK = DISPLAYLIST_BLOCK_SIZE / GFX_8_MEM_LINE;

    byte* dl = displaylist;
    //DLModeDef* modeDef = &dlDef.modes[0];
    byte prev_mode = 0;
    byte modeCount = 0;
    byte blockCount = 0;
    byte lineInBlock = 0;
    bool nextBlockFlag = false;
    byte b = 0; //, jvb = 0;
    #ifdef DEBUG
    ushort lineAddr = 0;
    #endif

    cprintf("%02X %02X %d\r\n", framebuffer, DISPLAYLIST_BLOCK_SIZE, LINES_PER_BLOCK);

    // Convert the modelines to DL instructions.
    while(dlDef.modes[modeCount].blank_lines != 0xFF) // for all of the mode definitions
    {
        byte line = 0;
        ushort* lms_addr = (ushort*)dlDef.modes[modeCount].buffer;

        //modeDef = &dlDef.modes[modeCount];

        #ifdef DEBUG
        lineAddr = lms_addr;
        #endif

        // Blank spaces
        for(; line < dlDef.modes[modeCount].blank_lines/8; ++line)
            *(dl++) = (byte)DL_BLK8;

        for(line = 0; line < dlDef.modes[modeCount].lines; ++line)
        {
            b = (byte)dlDef.modes[modeCount].mode;   // line is the graphics mode

            #ifdef DEBUG
            if(line && !(line%23))
            {
                cgetc();
                clrscr();
            }
            #endif

            if(!line)  // first line is an LMS to tell the DL where the framebuffer is
            {
                if(prev_mode != dlDef.modes[modeCount].mode)  // If the mode has changed
                    b = DL_LMS(b);
                prev_mode = dlDef.modes[modeCount].mode;
            }
            else       // Other than first line
            {
                nextBlockFlag = (bool)(!((ushort)line % LINES_PER_BLOCK));

                if (nextBlockFlag) // If our current line goes over the block boundary
                {
                    ++blockCount;
                    lineInBlock = 0;

                    // Add an LMS instruction to the DL and update the address to the next block
                    b = DL_LMS(b);
                    lms_addr = ((ushort)blockCount * DISPLAYLIST_BLOCK_SIZE) + framebuffer;
                    #ifdef DEBUG
                    lineAddr = lms_addr;
                    #endif
                }
            }

            #ifdef DEBUG
            cprintf("%d:%d %d %02X->%02X\r\n", line, lineInBlock, nextBlockFlag, lineAddr, lineAddr+(ushort)GFX_8_MEM_LINE);
            lineAddr += (ushort)GFX_8_MEM_LINE;
            #endif

            ++lineInBlock;

            if(dlDef.modes[modeCount].dli)
            {
                b = DL_DLI(b);
                //jvb = (byte)DL_DLI(DL_JVB);
                // Disable to DLI bit setting for follow on lines.
                //dlDef.modes[modeCount].dli = 0;
            }
            //else
            //    jvb = DL_JVB;

            *(dl++) = b;

            if(IS_LMS(b))
            {
                *((unsigned*)dl) = (unsigned)lms_addr;  // Add the address
                dl += 2;
            }
        }

        ++modeCount;
    }  // for all of the mode definitions

    // Add the JVB and perform the DLI if needed
    //if(dlDef.modes[modeCount].dli)
    //    b = (byte)DL_DLI(DL_JVB);
    //else
    //    b = DL_JVB;

    *(dl++) = DL_JVB;
    *((unsigned*)dl) = (unsigned)dlDef.address;
    dl+=2;

    #ifdef DEBUG
    printDList("GenDL");
    #endif
    #endif

    #undef DEBUG
}

void enableConsole()
{
    switch(CURRENT_MODE)
    {
        case GRAPHICS_0: // {0, DL_CHR40x8x1, 1, 0, CONSOLE_MEM}
            break;
        case GRAPHICS_8: // {8, DL_MAP320x1x1, 211, 0, 0}
        {
            CURRENT_MODE |= GRAPHICS_CONSOLE_EN;

            makeDisplayList(CURRENT_MODE);

            //OS.sdlst = gfxState.dl.address;
            ANTIC.nmien = 0x40;

            POKE(0x2BF, 5);
        }
        break;
        case GRAPHICS_9:
        case GRAPHICS_10:
        case GRAPHICS_11:
        {
            CURRENT_MODE |= GRAPHICS_CONSOLE_EN;

            makeDisplayList(CURRENT_MODE);

            //OS.sdlst = gfxState.dl.address;
            OS.vdslst = disable_9_dli;
            ANTIC.nmien = 0x80 | 0x40;

            POKE(0x2BF, 5);
        }

    }
}

void disableConsole()
{
    switch(CURRENT_MODE) // ^ GRAPHICS_CONSOLE_EN)
    {
        case GRAPHICS_0: // {0, DL_CHR40x8x1, 1, 0, CONSOLE_MEM}
            break;
        case GRAPHICS_8_CONSOLE: // {8, DL_MAP320x1x1, 211, 0, 0}
        case GRAPHICS_9_CONSOLE:
        case GRAPHICS_10_CONSOLE:
        case GRAPHICS_11_CONSOLE:
        {
            CURRENT_MODE &= (byte)~GRAPHICS_CONSOLE_EN;

            makeDisplayList(CURRENT_MODE);
            //POKEW(SDLSTL, gfxState.dl.address);

            ANTIC.nmien = 0x40;
            OS.vdslst = VDSLIST_STATE;

            POKE(0x2BF, 0);
        }
    }
}

// Shows the contents of a display list.
// name - simply used in the output header so you can tell which DL is which on the console.
// mloc - the location of the DL in memory 
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

#else
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
    if(!dlInfo->address)
        dlInfo->address = malloc_constrianed(1024, 1024);
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
                *((unsigned*)dlCmd) = (unsigned)lms_addr;  // Add the address
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
    cgetc();
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

            OS.sdlst = gfxState.dl.address;
            ANTIC.nmien = 0x40;

            POKE(0x2BF, 5);
        }
        break;
        case GRAPHICS_9:
        case GRAPHICS_10:
        case GRAPHICS_11:
        {
            gfxState.mode |= GRAPHICS_CONSOLE_EN;

            makeDisplayList(gfxState.mode, &gfxState.buffer, &gfxState.dl);

            OS.sdlst = gfxState.dl.address;
            OS.vdslst = disable_9_dli;
            ANTIC.nmien = 0x80 | 0x40;

            POKE(0x2BF, 5);
        }

    }
}

void disableConsole()
{
    switch(gfxState.mode) // ^ GRAPHICS_CONSOLE_EN)
    {
        case GRAPHICS_0: // {0, DL_CHR40x8x1, 1, 0, CONSOLE_MEM}
            break;
        case GRAPHICS_8_CONSOLE: // {8, DL_MAP320x1x1, 211, 0, 0}
        case GRAPHICS_9_CONSOLE:
        case GRAPHICS_10_CONSOLE:
        case GRAPHICS_11_CONSOLE:
        {
            gfxState.mode &= (byte)~GRAPHICS_CONSOLE_EN;

            makeDisplayList(gfxState.mode, &gfxState.buffer, &gfxState.dl);
            POKEW(SDLSTL, gfxState.dl.address);
            //OS.sdlst = gfxState.dl.address;

            ANTIC.nmien = 0x40;
            OS.vdslst = VDSLIST_STATE;

            POKE(0x2BF, 0);
        }
    }
}

void setGraphicsMode(byte mode)
{
    if(mode == gfxState.mode)
        return;

    if(!gfxState.buffer.size)
    {
        // don't free the display list memory.  It will be persistent.

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
            POKE(0x2BF, 26);
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

#endif