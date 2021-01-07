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
void readPBMIntoGfx8(int fd, void* dmem)
{
    const unsigned MAX_SIZE = 40 * 102; // Max size of blocks (4080 bytes)
    unsigned numbytes = 0;
    unsigned numread = 0;
    
    readHeader(fd);

    numbytes = (w / 8) * h;

    // printf("numbytes=%d\n", numbytes);

    // Have to divide the reads so they are on 4K bounderies
    while(numbytes)
    {
        numread = numbytes > MAX_SIZE ? MAX_SIZE : numbytes;
        read(fd, dmem, numread);
        (byte*)dmem += 0x1000; // Move to the next 4K block.

        numbytes -= numread;
    }
}

// Reads a file from fb and writes numbytes of it into dmem.
// Assumes destination will be Gfx9 formatted
void readPGMIntoGfx9(int fd, void* tmem, void* dmem)
{
    //const unsigned TMAX_SIZE = 0x1000; // Max size of temp buffer (2720 bytes) about 2/3rds
    const unsigned TMAX_SIZE = 80 * 34; // Max size of temp buffer (2720 bytes) about 2/3rds
    void* next_dmem = (byte*)dmem + 0x1000;
    unsigned numread, i;
    unsigned numbytes = 0;

    readHeader(fd);

    numbytes = w * h;

    // printf("numbytes=%d\n", numbytes);

    count = readLine(fd);
    mxp = 0;
    // printf("## %d: ", count);
    // for(i = 0; i < count; i++)
    //     printf("%02x ", buff[i]);
    // printf("\n");

    // Parse the maximum pixel value
    {
        int i, dc = 0;
        for(i = count-1; i >=0; i--)
        {
            unsigned digit = (unsigned)(buff[i] - ASC_0);
            mxp += pow10(digit, (unsigned)dc);
            // printf("%d, %d: %02x %d %d\n", i, dc, buff[i], digit, *v);
            dc++;
        } 
    }

    // printf("max pixel value = %d\n", mxp);

    while(numread = read(fd, tmem, TMAX_SIZE))
    {
        // Check if we moved passed the next memory block.  If so, switch to it.
        if((byte*)dmem + numread/2 > next_dmem)
        {
            dmem = next_dmem;
            (byte*)next_dmem += 0x1000;
        }

        // printf("%02X + %02x ", dmem, numread/2);

        // Process
        for(i = 0; i < numread/2; i++)
            ((byte*)dmem)[i] |= (( ((byte*)tmem)[i*2] & 0xF0) | (( ((byte*)tmem)[i*2+1] & 0xF0) >> 4));

        (byte*)dmem += numread/2;

        // printf("= %02X\n", dmem);
    }
}

void image_file_type(char* filename)
{
    byte len = strlen(filename);
    IMAGE_FILE_TYPE = 0;

    if(len > 4)  // ext plus .
    {
        char* ext = 0x0;
        int i;

        // try to get the extension
        for(i = 0; i < len; ++i)
        {
            if(filename[i] == 0x0E)
            {
                if((len - (i+1)) > 2)
                {
                    ext = filename + i + 1;
                    break;
                }
            }
        }

        if(ext)
        {
            if(ext[0] == 'p')
            {
                if(ext[1] == 'b')
                    IMAGE_FILE_TYPE = FILETYPE_PBM;
                if(ext[1] == 'g')
                    IMAGE_FILE_TYPE = FILETYPE_PGM;                
            }
        }
    }
}

#if 0
byte read_image_file(char* filename, void* tmem, void* dmem)
{
    byte status = 0;
    byte len = strlen(filename);
    if(len > 4)  // ext plus .
    {
        int fd = open(filename, O_RDONLY);
        if(fd >= 0)
        {
            #ifdef JUST_LOAD
            status = FILETYPE_PBM;
            readPBMIntoGfx8(fd, dmem);
            #else
            int i;
            byte* ext = 0x0;

            // try to get the extension
            for(i = 0; i < len; ++i)
            {
                if(filename[i] == 0x0E)
                    if((len - (i+1)) == 3)
                    {
                        ext = filename + i + 1;
                        break;
                    }
            }

            if(ext)
            {
                if(strncmp(ext, "pbm", 40))
                {
                    status = FILETYPE_PBM;
                    readPBMIntoGfx8(fd, dmem);
                }
                if(strncmp(ext, "pgm", 40))
                {
                    status = FILETYPE_PGM;
                    readPGMIntoGfx9(fd, tmem, dmem);
                }
            }
            else
            {
                        gotoy(10);
        cprintf("+++++ %s", filename);

            }
            #endif
        }

        close(fd);
    }

    return status;
}
#endif