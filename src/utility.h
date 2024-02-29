// Copyright (C) 2021 Brad Colbert

#ifndef UTILITY_H
#define UTILITY_H

#include "types.h"

void pause(const char* message);
void internal_to_atascii(char* buff, byte len);
void atascii_to_internal(char* buff, byte len);

#if 0
#include <stdlib.h>

typedef struct _MemSeg
{
    void* addr;
    size_t size;
    size_t block_size; // the size of the blocks in the segment
} MemSeg;

typedef MemSeg** MemSegParray;

#define MAX_NUM_SEGS 8
typedef struct _MemSegs
{
    byte num;
    void* start;
    void* end;
    size_t size;
    MemSeg segs[MAX_NUM_SEGS];
} MemSegs;

void pause(const char* message);
void* nextBoundary(void* start, unsigned bound);
void* malloc_constrianed(size_t size, size_t fence);
size_t allocSegmentedMemory(size_t block_size, size_t num_blocks, size_t boundary, MemSegs* memsegs);
void freeSegmentedMemory(MemSegs* memsegs);
void printMemSegs(const MemSegs* memsegs);
#endif

#endif