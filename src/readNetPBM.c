// Copyright (C) 2021 Brad Colbert

#define USE_ORIGINAL
#ifndef USE_ORIGINAL
#else
// Read routines for the NetPBM formats (PBM, PGM, so far)
#include "readNetPBM.h"
#include "types.h"
#include "consts.h"
#include "graphics.h"

#include <conio.h>

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

// Defines
#define BYTES_PER_LINE 40

// Globals
byte IMAGE_FILE_TYPE = 0;
extern byte buff[];
extern byte framebuffer[];      // defined in graphics.c

// Locals
int count;
byte b;
int w = 0;
int h = 0;
int mxp = 16; // max pixel value

// Simple power of ten function
unsigned pow10(unsigned val, byte exp)
{
    unsigned v = val;
    int i;
    for(i = 0; i < exp; i++)
        v *= 10;

    return v;
}

//
unsigned readLine(int fd)
{
    count = 0;
    while(1)
    {
        if(!read(fd, &b, 1))
            return count;

        if(b == ASC_LF)
            return count;

        buff[count++] = b;
    }
}

// Convert buff into integer
void parseWidthHeight()
{
    int* v = &h;
    int i, dc;
    dc = 0;
    for(i = count-1; i >=0; i--)
    {
        if(buff[i] == ASC_SPC)
        {
            v = &w;
            dc = 0;
        }
        else
        {
            unsigned digit = (unsigned)(buff[i] - ASC_0);
            *v += pow10(digit, (unsigned)dc);
            dc++;           
        }
    }
}

//
unsigned readComment(int fd)
{
    while(1)
    {
        READ_BYTE(fd, b)
        if(b == ASC_LF)
            break;
    }

    return 0;
}

int readHeader(int fd)
{
    count = 0;

    // Read the header
    count = readLine(fd);

    if(count != 2)
        return -2;

    count = 0;

    // Check for comment
    count = readLine(fd);

    if(buff[0] == ASC_HASH)  // Just read a comment, read more
        count = readLine(fd);

    parseWidthHeight();
}

// Reads a file from fb and writes numbytes of it into dmem.
// Assumes destination will be Gfx8 formatted
void readPBM(int fd)
{
    const ushort bytes_per_line = 40;
    const ushort lines = 220;
    ushort buffer_start;
    ushort block_size;
    ushort lines_per_block;
    ushort dl_block_size;
    ushort ttl_buff_size;
    ushort read_size;
    
    readHeader(fd);

    buffer_start = framebuffer;
    block_size = DISPLAYLIST_BLOCK_SIZE;
    lines_per_block = (ushort)(block_size/bytes_per_line);
    dl_block_size = lines_per_block * bytes_per_line;
    ttl_buff_size = lines * bytes_per_line;
    read_size = dl_block_size;

    while(ttl_buff_size > 0)
    {
        if(read_size > ttl_buff_size)
            read_size = ttl_buff_size;

        if(read(fd, buffer_start, read_size) < 0)
        {
            show_console();
            cprintf("Error reading\n\r");
            break;
        }

        buffer_start = buffer_start + block_size;
        ttl_buff_size = ttl_buff_size - read_size;
    }
}

// Reads a file from fb and writes numbytes of it into dmem.
// Assumes destination will be Gfx9 formatted
void readPGM(int fd)
{
    #if OLD_CODE
    unsigned numbytes = 0;
    unsigned count = 0;
    byte segcount = 0;
    
    memset(buff, 0, 255);
    readHeader(fd);

    numbytes = w * h;

    memset(buff, 0, 255);
    count = readLine(fd);
    mxp = 0;

    // Parse the maximum pixel value
    {
    int i;
    for(i = 0; i < count; ++i)
    {
        unsigned digit = (unsigned)(buff[i] - ASC_0);
        //cprintf("%d %d ", i, buff[i]);
        mxp += pow10(digit, count - (i+1));
    }
    } 

    while(gfxState.buffer.segs[segcount].size > 0)
    {
        MemSeg* seg = &gfxState.buffer.segs[segcount];
        size_t blocksInSeg = seg->size / seg->block_size;
        size_t segSize = blocksInSeg * seg->block_size;
        void* workBuffer = calloc(seg->block_size * 2, 1);

        if(workBuffer)
        {
            byte* fb = seg->addr;
            size_t blocks_ttl = 0;

            while(blocks_ttl < blocksInSeg)
            {
                size_t n_read = 0;
                // size_t n = seg->block_size * 2 * 10;

                // if(blocks_ttl + (n/2) > blocksInSeg)
                //     n = blocksInSeg - (blocks_ttl + (n/2));

                n_read = read(fd, workBuffer, seg->block_size * 2);
                if(n_read)
                {
                    int i;
                    for(i = 0; i < n_read/2; ++i)
                    {
                        size_t segIdx = (blocks_ttl * seg->block_size) + i;
                        ((byte*)seg->addr)[segIdx] = (( ((byte*)workBuffer)[i*2] & 0xF0) | (( ((byte*)workBuffer)[i*2+1] & 0xF0) >> 4));
                    }
                    // blocks_ttl += n_read / (2 * seg->block_size);
                    ++blocks_ttl;
                }
                else
                    break;
            }

            free(workBuffer);
        }

        ++segcount;
    }
    #else
    const ushort bytes_per_line = 40;
    const ushort lines = 220;
    const ushort lines_per_block = (ushort)(DISPLAYLIST_BLOCK_SIZE/bytes_per_line);
    const ushort dl_block_size = lines_per_block * bytes_per_line;
    ushort buffer_start;
    ushort ttl_buff_size;
    ushort read_size;
    
    readHeader(fd);

    // Parse the maximum pixel value
    {
    int i;
    for(i = 0; i < count; ++i)
    {
        unsigned digit = (unsigned)(buff[i] - ASC_0);
        mxp += pow10(digit, count - (i+1));
    }
    } 

    buffer_start = framebuffer;
    ttl_buff_size = lines * bytes_per_line;
    read_size = dl_block_size;

    while(ttl_buff_size > 0)
    {
        if(read_size > ttl_buff_size)
            read_size = ttl_buff_size;

        if(read(fd, buffer_start, read_size) < 0)
        {
            show_console();
            cprintf("Error reading\n\r");
            cgetc();
            break;
        }

        buffer_start = buffer_start + DISPLAYLIST_BLOCK_SIZE;
        ttl_buff_size = ttl_buff_size - read_size;
    }

    #endif
}
#endif