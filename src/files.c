// Copyright (C) 2021 Brad Colbert
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

// externals
extern GfxDef gfxState;

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
}

// load a file into the graphics frame buffer
byte loadFile(const char filename[])
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
                size_t n = 0;
                byte i = 0;
                byte maj = 0, min = 0, bld = 0;
                byte gfx_mode = 0;
                size_t totalBytesCopied = 0;
                byte segCount = 0;
                MemSeg* seg = &gfxState.buffer.segs[segCount];
                size_t dest_buff_size = (seg->size / seg->block_size) * seg->block_size;
                size_t dest_used = 0;

                #ifdef DEBUG_FILELOAD
                clrscr();
                gotoxy(0,0);
                printMemSegs(&gfxState.buffer);
                cgetc();
                #endif

                // Read the version #
                n = read(fd, (void*)&maj, 1);
                n = read(fd, (void*)&min, 1);
                n = read(fd, (void*)&bld, 1);

                // Version check.  If we are trying to load a file that is > ours
                // we may not be able to.  Warning for now.
                if((maj < MAJOR_VERSION) || (min < MINOR_VERSION) || (bld < BUILD_VERSION))
                {
                    reset_console();
                    cprintf("Warning version: %d %d %d", maj, min, bld);
                    //close(fd);
                    //return 0;
                }

                // Read the graphics mode
                n = read(fd, &gfx_mode, 1);
                if(n < 1)
                    break;

                setGraphicsMode(gfx_mode);
                disableConsole();
                #ifdef DEBUG_FILELOAD
                enableConsole();
                #endif

                #ifdef DEBUG_FILELOAD
                gotoxy(0,0);clrscr();
                cprintf("Read the graphics mode %d\n\r", gfx_mode);
                cgetc();
                #endif

                #ifdef DEBUG_FILELOAD
                gotoxy(0,0);clrscr();
                cprintf("Memory segments\n\r", gfx_mode);
                while(seg->size)
                {
                    if(segCount)
                        cputs(", ");
                    cprintf("%d: %p -> %p (%d)", segCount, seg->addr, (void*)((unsigned)seg->addr + seg->size), seg->size);
                    ++segCount;
                    seg = &gfxState.buffer.segs[segCount];
                }
                cgetc();
                segCount = 0;
                seg = &gfxState.buffer.segs[segCount];
                #endif

                while(n > 0)
                {
                    // Read the type token
                    byte type;
                    unsigned mem_loc;
                    size_t size;

                    n = read(fd, (void*)&type, 1);
                    if(n < 1)
                        break;
                    #ifdef DEBUG_FILELOAD
                    gotoxy(0,0);clrscr();
                    cprintf("Read the type %d\n\r", type);
                    cgetc();
                    #endif

                    if((maj == 1) && (min == 0) && (bld == 0))
                    {
                        n = read(fd, &mem_loc, 2);
                        if(n < 1)
                            break;
                        #ifdef DEBUG_FILELOAD
                        cprintf("Read the mem location %02X\n\r", mem_loc);
                        cgetc();
                        #endif
                    }

                    n = read(fd, &size, 2);
                    if(n < 1)
                        break;
                    #ifdef DEBUG_FILELOAD
                    gotoxy(0,0);clrscr();
                    cprintf("Read the size %d\n\r", size);
                    cgetc();
                    #endif

                    // for now, ignore any display list, just read and throw away.
                    // read the image and divy up into our memory footprint.
                    switch(type)
                    {
                        case MEM_TOKEN:  // Memory
                        {
                            size = (size / seg->block_size) * seg->block_size;  // in block units
                            while(size > 0)
                            {
                                #ifdef DEBUG_FILELOAD
                                gotoxy(0,0);clrscr();
                                cprintf("Writing %d bytes\n\r", size);
                                cgetc();
                                #endif

                                if(size < dest_buff_size - dest_used)
                                {
                                    #ifdef DEBUG_FILELOAD
                                    cprintf("%d < %d - %d\n\r", size, dest_buff_size, dest_used);
                                    cgetc();
                                    #endif

                                    n = read(fd, (void*)((size_t)seg->addr + dest_used), size);
                                    if(n < 1)
                                        break;

                                    #ifdef DEBUG_FILELOAD
                                    cprintf("Wrote %db to %p (%d)\n\r", n, (void*)((size_t)seg->addr + dest_used), size);
                                    #endif

                                    dest_used += n;
                                }
                                else
                                {
                                    #ifdef DEBUG_FILELOAD
                                    cprintf("%d: %d >= %d - %d\n\r", segCount, size, dest_buff_size, dest_used);
                                    cgetc();
                                    #endif

                                    n = read(fd, (void*)((size_t)seg->addr + dest_used), dest_buff_size - dest_used);
                                    if(n < 1)
                                        break;

                                    #ifdef DEBUG_FILELOAD
                                    cprintf("Wrote %db to %p (%d) %d rem\n\r", n, (void*)((size_t)seg->addr + dest_used), dest_buff_size - dest_used, size-n);
                                    #endif

                                    ++segCount;
                                    seg = &gfxState.buffer.segs[segCount];
                                    dest_buff_size = (seg->size / seg->block_size) * seg->block_size;
                                    dest_used = 0;

                                    #ifdef DEBUG_FILELOAD
                                    cprintf("New seg[%d] %p of %d\n\r", segCount, seg->addr, dest_buff_size);
                                    #endif
                                }


                                size -= n;                                

                                #ifdef DEBUG_FILELOAD
                                cgetc();
                                #endif
                            }
                        }
                        break;

                        default:
                        {
                            void* temp = malloc(size);
                            n = read(fd, temp, size);
                            free(temp);

                            #ifdef DEBUG_FILELOAD
                            gotoxy(0,0);clrscr();
                            cprintf("Read %d of data (DL/DLI)", n);
                            cgetc();
                            #endif
                        }
                        break;
                    } // switch type
                } // while n
            }
            break;
            
            case FILETYPE_PBM:
            case FILETYPE_PGM:
                //disableConsole();

                switch(file_type)
                {
                    case FILETYPE_PBM:
                        disableConsole();
                        setGraphicsMode(GRAPHICS_8);
                        readPBM(fd);
                        break;
                    case FILETYPE_PGM:
                        disableConsole();
                        setGraphicsMode(GRAPHICS_9);
                        readPGM(fd);
                        break;
                };
            break; 
        }

        close(fd);
    }
    else
        cprintf("ERROR: Problem opening file %s", filename);

    return 0;
}
#endif