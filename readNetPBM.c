// Copyright (C) 2021 Brad Colbert

// Read routines for the NetPBM formats (PBM, PGM, so far)
#include "readNetPBM.h"
#include "types.h"
#include "consts.h"

#include <conio.h>

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

// Defines
#define BYTES_PER_LINE 40

// Globals
byte IMAGE_FILE_TYPE = 0;

// Locals
int count;
byte b, buff[256];
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
        {
            //printf("\n");
            break;
        }
        //printf("%02x ", (char)b);
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
void readPBMIntoGfx8(int fd, byte* dmem)
{
    const unsigned MAX_SIZE = BYTES_PER_LINE * 102; // Max size of blocks (4080 bytes)
    unsigned numbytes = 0;
    unsigned numread = 0;

    //memset(dmem, 0x55, 8800 + 32);//+0x0400);
    //cgetc();

    readHeader(fd);

    numbytes = (w / 8) * h;

    //cprintf("%d\n\r", numbytes);
    //cgetc();

    // Have to divide the reads so they are on 4K bounderies
    while(numbytes)
    {
        numread = numbytes > MAX_SIZE ? MAX_SIZE : numbytes;
        read(fd, dmem, numread);
        dmem += 0x1000; // Move to the next 4K block.

        numbytes -= numread;
    }
}

// Reads a file from fb and writes numbytes of it into dmem.
// Assumes destination will be Gfx9 formatted
void readPGMIntoGfx9(int fd, byte* tmem, byte* dmem)
{
    const unsigned TMAX_SIZE = BYTES_PER_LINE * 68; // Max size of temp buffer (2560 bytes) about 2/3rds
    byte* next_dmem = (byte*)dmem + 0x1000;
    unsigned numread, i;
    unsigned numbytes = 0;

    readHeader(fd);

    numbytes = w * h;

    count = readLine(fd);
    mxp = 0;

    // Parse the maximum pixel value
    {
        int i, dc = 0;
        for(i = count-1; i >=0; i--)
        {
            unsigned digit = (unsigned)(buff[i] - ASC_0);
            mxp += pow10(digit, (unsigned)dc);
            dc++;
        } 
    }

    while(numread = read(fd, tmem, TMAX_SIZE))
    {
        // Check if we moved passed the next memory block.  If so, switch to it.
        if((byte*)dmem + numread/2 > next_dmem)  // half because there are 4 bits per pixel.  Each byte is two pixels in Gr9
        {
            dmem = next_dmem;
            next_dmem += 0x1000;
        }
        /*
        unsigned remaining = 
        if(!((dmem + (numread/2)) % 0x1000))
            dmem
        */

        // Process
        for(i = 0; i < numread/2; i++)
            ((byte*)dmem)[i] |= (( ((byte*)tmem)[i*2] & 0xF0) | (( ((byte*)tmem)[i*2+1] & 0xF0) >> 4));

        dmem += numread/2;
    }
}
