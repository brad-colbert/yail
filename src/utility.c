// Copyright (C) 2021 Brad Colbert

#include "utility.h"
#include "types.h"

#include <conio.h>

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#if 0

//
void* nextBoundary(void* start, unsigned bound)
{
    unsigned start_bound = (unsigned)start + bound;
    unsigned rem = start_bound % bound;
    return (void*)(start_bound - rem);
}

// A really sketchy function to allocate memory the doesnt cross boundaries.
void* malloc_constrianed(size_t size, size_t fence)
{
    // First allocate a block of the size and see if the tail crosses.
    // If so, free it and then reallocate based on the delta to make the
    // memory go to the fence.  Then allocate again which should be off
    // the fence and free the filler.
    void* mem = malloc(size);
    void* boundMem = NULL;
    void* filler = NULL;
    size_t fillerSize = 0;

    while((unsigned)mem % fence)
    {
        boundMem = nextBoundary(mem, fence);
        fillerSize = (size_t)boundMem - (size_t)mem;

        free(mem);

        if(fillerSize)
        {
            if(fillerSize < 5)
                filler = malloc(fillerSize + fence - 4); // malloc overhead
            else
                filler = malloc(fillerSize - 4);
        }

        mem = malloc(size);

        free(filler);

        #ifdef DEBUG_MEMORY_CODE
        cprintf("%p -> %p %d\n\r", filler, mem, fillerSize);
        #endif
    }

    #ifdef DEBUG_MEMORY_CODE
    cgetc();
    #endif

    return mem;
}

#ifdef DEBUG_MEMORY_CODE
void printMemSegs(const MemSegs* memsegs)
{
    byte i = 0;
    cprintf("------------------------------\n\r");
    cprintf("%d: %p %p -> %d\n\r", memsegs->num, memsegs->start, memsegs->end, memsegs->size);
    for(; i < MAX_NUM_SEGS; ++i)
        cprintf("(%d) %p %d\n\r", i, memsegs->segs[i].addr, memsegs->segs[i].size);
}
#endif

#define MALLOC_OVERHEAD 4
size_t allocSegmentedMemory(size_t block_size, size_t num_blocks, size_t boundary, MemSegs* memsegs)
{
    size_t size = block_size * num_blocks;
    size_t waste = size / boundary;
    void* next_seg = NULL;

    memsegs->num = 0;
    
    size += waste * block_size;

    memsegs->size = size;
    memsegs->start = malloc(size);
    memsegs->segs[0].addr = memsegs->start;
    memsegs->end = (void*)((size_t)memsegs->segs[0].addr + size);

    
    next_seg = nextBoundary(memsegs->start, boundary);
    memsegs->segs[0].size = (((size_t)next_seg - (size_t)memsegs->segs[0].addr)/block_size)*block_size;
    memsegs->segs[0].block_size = block_size;
    ++memsegs->num;

    while((memsegs->num < MAX_NUM_SEGS) && (next_seg < memsegs->end))
    {
        memsegs->segs[memsegs->num].addr = next_seg;
        next_seg = nextBoundary(memsegs->segs[memsegs->num].addr, boundary);
        if(next_seg >= memsegs->end)
            next_seg = memsegs->end;
        memsegs->segs[memsegs->num].size = (size_t)next_seg - (size_t)memsegs->segs[memsegs->num].addr;
        memsegs->segs[memsegs->num].block_size = block_size;
        ++memsegs->num;
    }
    
    #ifdef DEBUG_MEMORY_CODE
    printMemSegs(memsegs);
    cgetc();
    #endif

    return memsegs->size;
}

void freeSegmentedMemory(MemSegs* memsegs)
{
    free(memsegs->segs[0].addr);

    memset(memsegs, 0, sizeof(MemSegs));
}

#endif