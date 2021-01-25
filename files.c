// Copyright (C) 2021 Brad Colbert

#include "files.h"
#include "console.h"
#include "readNetPBM.h"
#include "graphics.h"
#include "version.h"

#include <conio.h>

#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h> 

// Externs
extern byte GRAPHICS_MODE;

void save_file(const char filename[],
               struct dl_store dl_mem[MAX_N_DL],
               struct dli_store dli_mem[MAX_N_DLI],
               struct mem_store gfx_mem[MAX_N_MEM])
{
    int fd = open(filename, O_WRONLY);

    if(fd >= 0)
    {
        byte i = 0, b;

        // Write the version
        b = MAJOR_VERSION;
        write(fd, &b, 1);
        b = MINOR_VERSION;
        write(fd, &b, 1);
        b = BUILD_VERSION;
        write(fd, &b, 1);

        // Write the graphics mode
        b = GRAPHICS_MODE;
        write(fd, &b, 1);

        // Write the DLs
        while(i < MAX_N_DL)
        {
            if(dl_mem[i].size)
            {
                b = DL_TOKEN;
                write(fd, &b, 1);
                write(fd, &dl_mem[i].mem, sizeof(unsigned));  // Write the memory location
                write(fd, &dl_mem[i].size, sizeof(unsigned));
                write(fd, dl_mem[i].mem, dl_mem[i].size);
            }
            else
                break;

            ++i;
        }
        // Write the DLs
        i = 0;
        while(i < MAX_N_DLI)
        {
            if(dli_mem[i].size)
            {
                b = DLI_TOKEN;
                write(fd, &b, 1);
                write(fd, &dli_mem[i].mem, 2);  // Write the memory location
                write(fd, &dli_mem[i].size, sizeof(unsigned));
                write(fd, dli_mem[i].mem, dli_mem[i].size);
            }
            else
                break;

            ++i;
        }

        // Write the image
        i = 0;
        while(i < MAX_N_MEM)
        {
            if(gfx_mem[i].size)
            {
                b = MEM_TOKEN;
                write(fd, &b, 1);
                write(fd, &gfx_mem[i].mem, 2);  // Write the memory location
                write(fd, &gfx_mem[i].size, sizeof(unsigned));
                write(fd, gfx_mem[i].mem, gfx_mem[i].size);
            }
            else
                break;

            ++i;
        }

        //
        close(fd);
    }
}

byte load_file(const char filename[],
               byte* gfx_mode,
               struct dl_store dl_mem[MAX_N_DL],
               struct dli_store dli_mem[MAX_N_DLI],
               struct mem_store gfx_mem[MAX_N_MEM],
               byte set_graphics_mode)
{
    int fd = open(filename, O_RDONLY);
    if(fd >= 0)
    {
        byte file_type = image_file_type(filename);

        cprintf("Loading... %s", filename);

        switch(file_type)
        {
            case FILETYPE_YAI:
            {
                byte i = 0;
                byte n = 0, maj = 0, min = 0, bld = 0;

                // Read the version #
                n = read(fd, (void*)&maj, 1);
                n = read(fd, (void*)&min, 1);
                n = read(fd, (void*)&bld, 1);

                // Read the graphics mode
                n = read(fd, (void*)gfx_mode, 1);
                if(set_graphics_mode)
                    set_graphics(*gfx_mode);

                #ifdef DEBUG_FILELOAD
                gotoxy(0,0);
                cputs("Read the graphics mode\n\r");
                #endif

                // Read the DLs
                while(n > 0)
                {
                    // Read the type token
                    byte type;
                    unsigned mem_loc;
                    unsigned size;

                    n = read(fd, (void*)&type, 1);
                    if(n < 1)
                        break;
                    #ifdef DEBUG_FILELOAD
                    cprintf("Read the type %d\n\r", type);
                    #endif
                    n = read(fd, &mem_loc, 2);
                    if(n < 1)
                        break;
                    #ifdef DEBUG_FILELOAD
                    cprintf("Read the mem location %02X\n\r", mem_loc);
                    #endif
                    n = read(fd, &size, 2);
                    if(n < 1)
                        break;
                    #ifdef DEBUG_FILELOAD
                    cprintf("Read the size %d\n\r", size);
                    #endif
                    n = read(fd, (void*)mem_loc, size);
                    if(n < 1)
                        break;
                    #ifdef DEBUG_FILELOAD
                    cprintf("Read the data %d\n\r", n);
                    #endif

                    switch(type)
                    {
                        case DL_TOKEN:
                            dl_mem[i].size = size;
                            dl_mem[i].mem = (void*)mem_loc;
                            break;
                        case DLI_TOKEN:
                            dli_mem[i].size = size;
                            dli_mem[i].mem = (void*)mem_loc;
                            break;
                        case MEM_TOKEN:
                            gfx_mem[i].size = size;
                            gfx_mem[i].mem = (void*)mem_loc;
                            break;
                    }
                } // while n
            }
            break;
            
            case FILETYPE_PBM:
            case FILETYPE_PGM:
                reset_console();   // clear the console mem
                disable_console(); // disable the console (Gf9 DLI doesn't like file loads)
                graphics_clear();

                switch(file_type)
                {
                    case FILETYPE_PBM:
                        //set_graphics(GRAPHICS_8);
                        readPBMIntoGfx8(fd, (void*)MY_SCRN_MEM);
                        set_graphics(GRAPHICS_8);
                        break;
                    case FILETYPE_PGM:
                        set_graphics(GRAPHICS_9);
                        readPGMIntoGfx9(fd, (void*)MY_SCRN_MEM_TEMP, (void*)MY_SCRN_MEM);
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

byte image_file_type(const char filename[])
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