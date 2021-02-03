// Copyright (C) 2021 Brad Colbert

#ifndef UTILITY_H
#define UTILITY_H

#include "types.h"

#include <stdlib.h>

typedef struct _MemSeg
{
    void* addr;
    size_t size;
    size_t block_size; // the size of the blocks in the segment
} MemSeg;

typedef MemSeg** MemSegParray;

/*
typedef struct _MemSegs
{
    byte num;
    void* start;
    void* end;
    size_t size;
    MemSegParray segs;
} MemSegs;
*/

#define MAX_NUM_SEGS 8
typedef struct _MemSegs
{
    byte num;
    void* start;
    void* end;
    size_t size;
    MemSeg segs[MAX_NUM_SEGS];
} MemSegs;

void* nextBoundary(void* start, unsigned bound);
void* malloc_constrianed(size_t size, size_t fence);

size_t allocSegmentedMemory(size_t block_size, size_t num_blocks, size_t boundary, MemSegs* memsegs);
void freeSegmentedMemory(MemSegs* memsegs);
// void* aligned_malloc(size_t required_bytes, size_t alignment);
// void aligned_free(void *p);

void printMemSegs(const MemSegs* memsegs);
#endif