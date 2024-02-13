// Copyright (C) 2021 Brad Colbert
#define USE_ORIGINAL
#ifndef USE_ORIGINAL
#else
#include "files.h"
#include "console.h"
#include "readNetPBM.h"
#include "graphics.h"
#include "version.h"
#include "utility.h"

#include <conio.h>

#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h> 

typedef struct image_header
{
    unsigned char v1;
    unsigned char v2;
    unsigned char v3;
    unsigned char gfx;
    unsigned char memtkn;
    short size;
} ImageHeader;

//
typedef struct image_data
{
    ImageHeader header;
    byte* data;
} ImageData;

// externals
//extern GfxDef gfxState;
extern ImageData image;
extern byte buff[];

// returns a token based on the filetype determined from the extension
byte imageFileType(const char filename[])
{
    byte len = strlen(filename);

    if(len > 4)  // ext plus .
    {
        char* ext = 0x0;
        int i;

        // try to get the extension
        for(i = 0; i < len; ++i)
        {
            if((filename[i] == 0x0E) || (filename[i] == 0x2E))
            {
                if((len - (i+1)) > 2)
                {
                    ext = (char*)&filename[i + 1];
                    break;
                }
            }
        }

        if(ext)
        {
            if(toupper(ext[0]) == 'P')
            {
                if(toupper(ext[1]) == 'B')
                    return FILETYPE_PBM;
                if(toupper(ext[1]) == 'G')
                    return FILETYPE_PGM;                
            }
            else if(toupper(ext[0]) == 'Y')
                if(toupper(ext[1]) == 'A')
                    return FILETYPE_YAI;
        }
    }
}

void saveFile(const char filename[])
{
    #if 0
    int fd = open(filename, O_WRONLY);

    if(fd >= 0)
    {
        byte segCount = 0;
        MemSeg* seg = &gfxState.buffer.segs[segCount];
        byte i = 0, b;

        // Write the version
        b = MAJOR_VERSION;
        write(fd, &b, 1);
        b = MINOR_VERSION;
        write(fd, &b, 1);
        b = BUILD_VERSION;
        write(fd, &b, 1);

        // Write the graphics mode
        b = gfxState.mode & ~GRAPHICS_CONSOLE_EN;
        write(fd, &b, 1);

        // Write the DLs
        b = DL_TOKEN;
        write(fd, &b, 1);
        write(fd, &gfxState.dl.size, sizeof(unsigned));
        write(fd, gfxState.dl.address, gfxState.dl.size);

        // Write the image
        while(seg->size)
        {
            size_t size = (seg->size / seg->block_size) * seg->block_size;
            b = MEM_TOKEN;
            write(fd, &b, 1);
            write(fd, &size, sizeof(size_t));
            write(fd, seg->addr, size);

            #ifdef DEBUG_FILELOAD
            gotoxy(0,0);clrscr();
            cprintf("%d: Wrote %d\n\r", segCount, size);
            cgetc();
            #endif

            ++segCount;
            seg = &gfxState.buffer.segs[segCount];
        }

        //
        close(fd);
    }
    #endif
}

// load a file into the graphics frame buffer
byte load_image_file(const char filename[])
{
    int fd = open(filename, O_RDONLY);
    if(fd >= 0)
    {
        byte file_type = imageFileType(filename);

        //cprintf("Loading... %s", filename);

        switch(file_type)
        {
            case FILETYPE_YAI:
            {
                const ushort BYTES_PER_LINE = 40;
                const ushort LINES = 220;
                const ushort BUFFER_BLOCK_SIZE = 4080;
                byte* buffer = image.data;
                ushort buffer_block_remaining = BUFFER_BLOCK_SIZE;
                FileHeader header;
                byte block_type;
                unsigned block_size = 0;
                size_t ttl_bytes = 0;
                size_t n = 0;

                // Read the header
                n = read(fd, (void*)&header, sizeof(header));
                ttl_bytes += n;

                //cprintf("Read %d bytes %d %d %d\n\r", n, header.v1, header.v2, header.v3);

                //
                setGraphicsMode(header.gfx); // GRAPHICS_0);//

                // Read the next block of info.  We are assuming is a display list.
                while(n > 0)
                {
                    n = read(fd, (void*)&block_type, sizeof(block_type));
                    ttl_bytes += n;

                    switch(block_type)
                    {
                        case DL_TOKEN:
                        {
                            n = read(fd, (void*)&block_size, sizeof(block_size));
                            ttl_bytes += n;
                            n = read(fd, (void*)buff, block_size);
                            ttl_bytes += n;
                        }
                        break;

                        case MEM_TOKEN:
                        {
                            // Read how big this block is.  If the block is larger than the next memory block
                            // we need to adjust the size that we read.  We then need to read the remaining
                            // into the next memory block.  Going to cheat a little and assume that none of the
                            // blocks have more than 4K of data.
                            //unsigned next_mem_block = buffer + image_block_size;
                            n = read(fd, (void*)&block_size, sizeof(block_size));
                            ttl_bytes += n;

                            // Check if the data we have available is greater than the current buffer block
                            // size.  If so, we will read only enough data to fill the buffer block and then
                            // read the remaining into the next buffer block.
                            if(block_size > buffer_block_remaining)
                            {
                                //cprintf("%p: %d > %d\n\r", buffer, block_size, buffer_block_remaining);
                                // Read enough to fill the buffer block
                                n += read(fd, (void*)buffer, buffer_block_remaining);
                                
                                // Move the buffer pointer to the next block
                                buffer += buffer_block_remaining + 16;  // 16 means that the last line will not cross a 4K boundary

                                // Read the remaining data into the next buffer block
                                n += read(fd, (void*)buffer, block_size - buffer_block_remaining);

                                // Reset the buffer block remaining
                                //cprintf("%p: %d = %d - (%d - %d)\n\r", buffer, (BUFFER_BLOCK_SIZE - (block_size - buffer_block_remaining)), BUFFER_BLOCK_SIZE, block_size, buffer_block_remaining);
                                buffer += (block_size - buffer_block_remaining);
                                buffer_block_remaining = BUFFER_BLOCK_SIZE - (block_size - buffer_block_remaining);
                            }
                            else
                            {
                                //cprintf("%p: %d !> %d\n\r", buffer, block_size, buffer_block_remaining);
                                n += read(fd, (void*)buffer, block_size);
                                buffer += block_size;
                                //cprintf("%p: %d = %d - %d\n\r", buffer, (buffer_block_remaining - block_size), buffer_block_remaining, block_size);
                                buffer_block_remaining -= block_size;
                            }

                            ttl_bytes += n;
                            //cprintf("\n\r", block_size);
                        }
                        break;
                        default:
                            n = 0;
                    }
                }

                return n;
            }
            break;
            
            case FILETYPE_PBM:
            case FILETYPE_PGM:
            #if 0
                switch(file_type)
                {
                    case FILETYPE_PBM:
                        hide_console();
                        setGraphicsMode(GRAPHICS_8);
                        readPBM(fd);
                        break;
                    case FILETYPE_PGM:
                        hide_console();
                        setGraphicsMode(GRAPHICS_9);
                        readPGM(fd);
                        break;
                };
                #endif
            break; 
        }

        close(fd);
    }
    else
    {
        cprintf("ERROR: Problem opening file %d:*%s*", strlen(filename), filename);
        cgetc();
    }

    return 0;
}
#endif