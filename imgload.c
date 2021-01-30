// Copyright (C) 2021 Brad Colbert

#include "console.h"
#include "graphics.h"
#include "files.h"
#include "utility.h"

#include <atari.h>
#include <conio.h>
#include <peekpoke.h>

#include <stdlib.h>
#include <string.h>

//
extern byte GRAPHICS_MODE;

//
#define MALLOC_OVERHEAD 4
extern GfxDef gfxState;
extern char* console_buff;

//
int main(int argc, char* argv[])
{
    byte i = 0;

    clrscr();
    // Initialize everything
    memset(&gfxState, 0, sizeof(GfxDef));
    gfxState.dl.address = NULL;

    saveCurrentGraphicsState();
    setGraphicsMode(GRAPHICS_8, 0);

    /* Test background
    while(gfxState.buffer.segs[i].size > 0)
    {
        memset(gfxState.buffer.segs[i].addr, 0x55, gfxState.buffer.segs[i].size);
        ++i;
    }
    */

    enableConsole();
    console_update();

    restoreGraphicsState();

    return 0;

    /*

    console_buff = OS.savmsc;

    saveCurrentGraphicsState();
    setGraphicsMode(GRAPHICS_8)333**;
    cgetc();

    while(gfxState.buffer.segs[i].size > 0)
    {
        memset(gfxState.buffer.segs[i].addr, 0x33, gfxState.buffer.segs[i].size);
        ++i;
    }

    for(i=0; i<80; i++)
        console_buff[i] = i;

    cgetc();
    enableConsole();
    cgetc();
    disableConsole();
    cgetc();

    restoreGraphicsState();
    return 0;
    */


    #if 0
    GfxDef gfxInfo;
    /*
    MemSegs segs;
    size_t n = computeMemorySegmentation((void*)0x6000, 220 * 40, 40, 0x1000, &segs);
    cgetc();
    clrscr();
    cprintf("** %d %p\n\r", n, &segs);
    printMemorySegmentationTable(&segs);
    freeMemorySegmentTable(&segs);
    
    cgetc();
    */
    clrscr();
    makeGraphicsDef(GRAPHICS_8, &gfxInfo);
    #endif


    #if 0
    // Clear the text and memory
    enable_console();
    cursor(1);
    reset_console();

    save_current_graphics_state();
    
    //memset(0x6000, 0x55, 0x6FFF);
    set_graphics(GRAPHICS_0);  // Start in Gfx 8 for giggles
    //graphics_clear();

    if(argc > 1)
    {
        struct dl_store dl_mem[MAX_N_DL];
        struct dli_store dli_mem[MAX_N_DLI];
        struct mem_store gfx_mem[MAX_N_MEM];

        fix_chars(argv[1]);
        load_file(argv[1], &GRAPHICS_MODE, dl_mem, dli_mem, gfx_mem, 1);
    }
    
    console_update();

    restore_graphics_state();
    #endif

    return 0;
}