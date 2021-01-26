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
    byte* filler = malloc(size);
    unsigned end = (unsigned)filler + size;
    unsigned pre_fence = ((unsigned)filler / fence) * fence;
    unsigned post_fence = pre_fence + fence;

    //cprintf("%p : %p : %p ==> %p\n\r", pre_fence, filler, post_fence, filler + size);

    if(end > post_fence)
    {
        // get the diff
        byte* mem;
        unsigned to_post_fence = post_fence - (unsigned)filler;
        to_post_fence -= 4;
        free(filler);
        filler = malloc(to_post_fence);
        //cprintf("*** (%02x) %p : %p : %p ==> %p\n\r", to_post_fence, pre_fence, filler, post_fence, filler + to_post_fence);

        // Allocate the unfenced mem
        mem = calloc(1, size);

        // Free the filler
        free(filler);

        filler = mem;

        //cprintf("%p : %p : %p ==> %p\n\r", pre_fence, filler, post_fence, filler + size);
    }

    return filler;
}

void* aligned_malloc(size_t required_bytes, size_t alignment)
{
    void* p1; // original block
    void** p2; // aligned block
    int offset = alignment - 1 + sizeof(void*);
    //if ((p1 = (void*)malloc(required_bytes + offset)) == NULL)
    if ((p1 = (void*)calloc(required_bytes + offset, 1)) == NULL)
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

#define MAX_NUM_SEGS 8
typedef struct _MemSegsX
{
    byte num;
    void* start;
    void* end;
    size_t size;
    MemSeg segs[MAX_NUM_SEGS];
} MemSegsX;

#define MALLOC_OVERHEAD 4
size_t allocSegmentedMemory(size_t block_size, size_t num_blocks, size_t boundary)//, MemSegs* segs)
{
    MemSegsX segs;
    //unsigned block_size = 40;
    //unsigned num_blocks = 3*190;
    unsigned len = block_size * num_blocks;
    //unsigned bound = 0x1000;
    void* mem = NULL;
    void* mem_orig = NULL;
    void* mem_bound = NULL;
    unsigned mem_resize = 0;
    unsigned mem_blocks = 0;
    unsigned blocks_used = 0;
    unsigned blocks_rem = num_blocks;

    memset(&segs, 0, sizeof(MemSegsX));

    clrscr();

    while(blocks_used < num_blocks)
    {
        mem = malloc(len);

        if(!mem)
            break;

        mem_orig = mem;
        mem_bound = nextBoundary(mem, boundary);
        mem_resize = ((unsigned)mem_bound - (unsigned)mem) - (MALLOC_OVERHEAD);

        if((mem_resize/block_size) > blocks_rem)
        {
            unsigned prev_mem_resize = mem_resize;
            mem_resize = blocks_rem * block_size;

        cprintf("%d=((%d/%d)-%d)*%d\n\r", mem_resize, prev_mem_resize, block_size, blocks_rem, block_size);
        }

        mem = realloc(mem, mem_resize);
        mem_blocks = mem_resize / block_size;
        blocks_used += mem_blocks;
        blocks_rem = num_blocks - blocks_used;

        cprintf("sz%d rz%d %p->%p us%d rm%d\n\r", len, mem_resize, mem, (void*)((unsigned)mem + mem_resize), blocks_used, blocks_rem);
        cgetc();

        len = blocks_rem * block_size;

        if(!segs.num)
            segs.start = mem;

        segs.segs[segs.num].addr = mem;
        segs.segs[segs.num].size = mem_resize;
        ++segs.num;
    }
    segs.end = (void*)((unsigned)mem + mem_resize);
    segs.size = (unsigned)segs.end - (unsigned)segs.start;
    
    return segs.size;
}

size_t computeMemorySegmentation(void* start_addr, size_t len, size_t alignment, size_t constraint, MemSegs* segs)
{
    //
    unsigned current_addr = (unsigned)start_addr;
    byte count = 0;
    size_t ttl = 0;
    size_t used = 0;
    size_t block_size = constraint;

    memset(segs, 0x0, sizeof(MemSegs));
    segs->start = start_addr;

    //
    while(ttl < len)
    {
        unsigned offset_addr = current_addr + constraint;
        unsigned rem = offset_addr % constraint;
        unsigned next_cnst = (unsigned)((int)offset_addr - (int)rem);
        MemSeg* seg = NULL;
       
        // calc number of alignments in block
        unsigned n = 0;

        if((current_addr + (len - ttl)) < next_cnst)
            block_size = len - ttl;
        else
            block_size = next_cnst - current_addr;

        n = block_size / alignment;

        cprintf("%02x %02x %02x %02x\n\r", current_addr, (current_addr + (len - ttl)), next_cnst, block_size);
        cgetc();

        // get memory used
        used = n * alignment;

        //cprintf("%d: %02x %02x %02x %02x\n\r", count, start_addr, offset_addr, rem, next_cnst);

        // make a segment
        seg = calloc(1, sizeof(MemSeg));
        seg->addr = (void*)current_addr;
        seg->size = used;

        // Add to the segments
        segs->segs = realloc(segs->segs, count+1);
        segs->segs[count] = seg;
        segs->end = (void*)((unsigned)current_addr + used);
     
        ++count;

        current_addr = next_cnst;
        ttl += used;

        segs->num = count;
    }

    segs->segs = realloc(segs->segs, count);
    segs->segs[count] = 0x0;
    segs->size = (size_t)((int)segs->end - (int)segs->start);

    //
    return segs->num;
}

void freeMemorySegmentTable(MemSegs* segs)
{
    byte count = 0;
    MemSeg* seg = segs->segs[0];
    while(seg)
    {
        free(seg);
        ++count;
        seg = segs->segs[count];
    }

    free(segs->segs);
}

void printMemorySegmentationTable(const MemSegs* segs)
{
    byte count = 0;
    MemSeg* seg = segs->segs[0];
    cprintf("%p -> %p: %d %d\n\r", segs->start, segs->end, segs->num, segs->size);
    while(seg)
    {
        cprintf("%p : %d\n\r", seg->addr, seg->size);

        ++count;
        seg = segs->segs[count];
    }
}
