// Copyright (C) 2021 Brad Colbert

#include "utility.h"
#include "types.h"

#include <conio.h>

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

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

#if 0
void* aligned_malloc(size_t required_bytes, size_t alignment)
{
    void* p1; // original block
    void** p2; // aligned block
    int offset = alignment - 1 + sizeof(void*);
    if ((p1 = (void*)malloc(required_bytes + offset)) == NULL)
    {
       return NULL;
    }
    p2 = (void**)(((size_t)(p1) + offset) & ~(alignment - 1));
    p2[-1] = p1;
    return p2;
}

void aligned_free(void *p)
{
    free(((void**)p)[-1]);
}
#endif

#if 1//def DEBUG_MEMORY_CODE
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
    memsegs->segs[0].size = (size_t)next_seg - (size_t)memsegs->segs[0].addr;
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


    #if 0
    unsigned len = block_size * num_blocks;
    void* mem = NULL;
    void* mem_orig = NULL;
    void* mem_bound = NULL;
    unsigned mem_resize = 0;
    unsigned mem_blocks = 0;
    unsigned blocks_used = 0;
    unsigned blocks_rem = num_blocks;

    memset(memsegs, 0, sizeof(MemSegs));

    #ifdef DEBUG_MEMORY_CODE
    clrscr();
    #endif

    while((memsegs->num < MAX_NUM_SEGS) && (blocks_used < num_blocks))
    {
        mem = calloc(1, len);

        if(!mem)
            break;

        mem_orig = mem;
        mem_bound = nextBoundary(mem, boundary);
        mem_resize = ((unsigned)mem_bound - (unsigned)mem) - (MALLOC_OVERHEAD);

        if((mem_resize/block_size) > blocks_rem)
        {
            unsigned prev_mem_resize = mem_resize;
            mem_resize = blocks_rem * block_size;
            #ifdef DEBUG_MEMORY_CODE
            cprintf("%d=((%d/%d)-%d)*%d\n\r", mem_resize, prev_mem_resize, block_size, blocks_rem, block_size);
            #endif
        }

        mem = realloc(mem, mem_resize);
        if(!mem)
            break;

        mem_blocks = mem_resize / block_size;
        blocks_used += mem_blocks;
        blocks_rem = num_blocks - blocks_used;

        #ifdef DEBUG_MEMORY_CODE
        cprintf("sz%d rz%d %p->%p us%d rm%d\n\r", len, mem_resize, mem, (void*)((unsigned)mem + mem_resize), blocks_used, blocks_rem);
        cgetc();
        #endif

        len = blocks_rem * block_size;

        if(!memsegs->num)
            memsegs->start = mem;

        memsegs->segs[memsegs->num].addr = mem;
        memsegs->segs[memsegs->num].size = mem_resize;
        memsegs->segs[memsegs->num].block_size = block_size; //(mem_resize / block_size) * block_size;
        ++memsegs->num;
    }
    memsegs->end = (void*)((unsigned)mem + mem_resize);
    memsegs->size = (unsigned)memsegs->end - (unsigned)memsegs->start;
    #endif
    
    #ifdef DEBUG_MEMORY_CODE
    printMemSegs(memsegs);
    cgetc();
    #endif

    return memsegs->size;
}

/*
typedef struct _MemSegs
{
    byte num;
    void* start;
    void* end;
    size_t size;
    MemSeg segs[MAX_NUM_SEGS];
} MemSegs;
*/
void freeSegmentedMemory(MemSegs* memsegs)
{
    /*
    byte i = 0;
    for(; i < MAX_NUM_SEGS; i++)
        if(memsegs->segs[i].size > 0)
            free(memsegs->segs[i].addr);
    */
    free(memsegs->segs[0].addr);

    memset(memsegs, 0, sizeof(MemSegs));
}