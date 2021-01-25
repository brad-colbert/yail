// Copyright (C) 2021 Brad Colbert

#include "utility.h"
#include "types.h"

#include <stdlib.h>

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
        mem = malloc(size);

        // Free the filler
        free(filler);

        filler = mem;

        //cprintf("%p : %p : %p ==> %p\n\r", pre_fence, filler, post_fence, filler + size);
    }

    return filler;
}