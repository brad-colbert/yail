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

//
int main(int argc, char* argv[])
{
    #if 0
    unsigned chunk_size = 40;
    unsigned num_chunks = 3*190;
    unsigned len = chunk_size*num_chunks;
    unsigned bound = 0x1000;
    void* mem = NULL; //malloc(len);
    void* mem_orig = NULL;
    void* mem_bound = NULL; //nextBoundary(mem, bound);
    unsigned mem_resize = 0; //((unsigned)mem_bound - (unsigned)mem) - (MALLOC_OVERHEAD);
    unsigned mem_chunks = 0;
    unsigned chunks_used = 0; //mem_used / chunk_size;
    unsigned chunks_rem = num_chunks; //num_chunks - chunks_used;

    clrscr();

    while(chunks_used < num_chunks)
    {
        mem = malloc(len);

        if(!mem)
            break;

        mem_orig = mem;
        mem_bound = nextBoundary(mem, bound);
        mem_resize = ((unsigned)mem_bound - (unsigned)mem) - (MALLOC_OVERHEAD);

        if((mem_resize/chunk_size) > chunks_rem)
        {
            unsigned prev_mem_resize = mem_resize;
            mem_resize = chunks_rem * chunk_size;

        cprintf("%d=((%d/%d)-%d)*%d\n\r", mem_resize, prev_mem_resize, chunk_size, chunks_rem, chunk_size);
        }

        mem = realloc(mem, mem_resize);
        mem_chunks = mem_resize / chunk_size;
        chunks_used += mem_chunks;
        chunks_rem = num_chunks - chunks_used;

        cprintf("sz%d rz%d %p->%p us%d rm%d\n\r", len, mem_resize, mem, (void*)((unsigned)mem + mem_resize), chunks_used, chunks_rem);
        cgetc();

        len = chunks_rem * chunk_size;
    }
    
    cprintf("%d\n\r", 4092/40);
    #endif

    /*
    MemSegs memsegs;
    DLDef dlDef;

    allocSegmentedMemory(40, 220, 4096, &memsegs);
    #ifdef DEBUG_MEMORY_CODE
    printMemSegs(&memsegs);

    cgetc();
    #endif

    makeDisplayList(GRAPHICS_8, &memsegs, &dlDef);
    #ifdef DEBUG_GRAPHICS
    cgetc();
    clrscr();
    printDList("DL", &dlDef);

    cgetc();
    #endif
    */
    GfxDef gfxInfo;
    makeGraphicsDef(GRAPHICS_8, &gfxInfo);
    return 0;


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