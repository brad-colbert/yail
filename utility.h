// Copyright (C) 2021 Brad Colbert

#ifndef UTILITY_H
#define UTILITY_H

#include "types.h"

#include <stdlib.h>

typedef struct _MemSeg
{
    void* addr;
    size_t size;
} MemSeg;

typedef MemSeg** MemSegParray;

typedef struct _MemSegs
{
    byte num;
    void* start;
    void* end;
    size_t size;
    MemSegParray segs;
} MemSegs;

void* nextBoundary(void* start, unsigned bound);
void* malloc_constrianed(size_t size, size_t fence);
void* aligned_malloc(size_t required_bytes, size_t alignment);
void aligned_free(void *p);
size_t computeMemorySegmentation(void* start_addr, size_t len, size_t alignment, size_t constraint, MemSegs* segs);
void freeMemorySegmentTable(MemSegs* segs);
void printMemorySegmentationTable(const MemSegs* segs);
size_t allocSegmentedMemory(size_t block_size, size_t num_blocks, size_t boundary);
#endif