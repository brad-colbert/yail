// Copyright (C) 2021 Brad Colbert

#include "files.h"
#include "graphics.h"
#include "version.h"

#include <conio.h>

#include <unistd.h>
#include <fcntl.h>
#include <string.h>

// Externs
extern byte GRAPHICS_MODE;

void save_file(const char filename[],
               struct dl_store dl_mem[MAX_N_DL],
               struct dli_store dli_mem[MAX_N_DLI],
               struct mem_store gfx_mem[MAX_N_MEM])
{
    int fp = open(filename, O_WRONLY);

    if(fp >= 0)
    {
        byte i = 0, b;

        // Write the version
        b = MAJOR_VERSION;
        write(fp, &b, 1);
        b = MINOR_VERSION;
        write(fp, &b, 1);
        b = BUILD_VERSION;
        write(fp, &b, 1);

        // Write the graphics mode
        b = GRAPHICS_MODE;
        write(fp, &b, 1);

        // Write the DLs
        while(i < MAX_N_DL)
        {
            if(dl_mem[i].size)
            {
                b = DL_TOKEN;
                write(fp, &b, 1);
                write(fp, &dl_mem[i].mem, sizeof(unsigned));  // Write the memory location
                write(fp, &dl_mem[i].size, sizeof(unsigned));
                write(fp, dl_mem[i].mem, dl_mem[i].size);
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
                write(fp, &b, 1);
                write(fp, &dli_mem[i].mem, 2);  // Write the memory location
                write(fp, &dli_mem[i].size, sizeof(unsigned));
                write(fp, dli_mem[i].mem, dli_mem[i].size);
            }
            else
                break;

            ++i;
        }

        // Write the DLs
        i = 0;
        while(i < MAX_N_MEM)
        {
            if(gfx_mem[i].size)
            {
                b = MEM_TOKEN;
                write(fp, &b, 1);
                write(fp, &gfx_mem[i].mem, 2);  // Write the memory location
                write(fp, &gfx_mem[i].size, sizeof(unsigned));
                write(fp, gfx_mem[i].mem, gfx_mem[i].size);
            }
            else
                break;

            ++i;
        }

        //
        close(fp);
    }
}

byte load_file(const char filename[],
               byte* gfx_mode,
               struct dl_store dl_mem[MAX_N_DL],
               struct dli_store dli_mem[MAX_N_DLI],
               struct mem_store gfx_mem[MAX_N_MEM],
               byte set_graphics_mode)
{
    int fp = open(filename, O_RDONLY);

    if(fp >= 0)
    {
        byte i = 0;
        byte n = 0, maj = 0, min = 0, bld = 0;

        // Read the version #
        n = read(fp, (void*)&maj, 1);
        n = read(fp, (void*)&min, 1);
        n = read(fp, (void*)&bld, 1);

        // Read the graphics mode
        n = read(fp, (void*)gfx_mode, 1);
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

            n = read(fp, (void*)&type, 1);
            if(n < 1)
                break;
            #ifdef DEBUG_FILELOAD
            cprintf("Read the type %d\n\r", type);
            #endif
            n = read(fp, &mem_loc, 2);
            if(n < 1)
                break;
            #ifdef DEBUG_FILELOAD
            cprintf("Read the mem location %02X\n\r", mem_loc);
            #endif
            n = read(fp, &size, 2);
            if(n < 1)
                break;
            #ifdef DEBUG_FILELOAD
            cprintf("Read the size %d\n\r", size);
            #endif
            n = read(fp, (void*)mem_loc, size);
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
        }

        close(fp);
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
            if(ext[0] == 'p')
            {
                if(ext[1] == 'b')
                    return FILETYPE_PBM;
                if(ext[1] == 'g')
                    return FILETYPE_PGM;                
            }
            else if(ext[0] == 'y')
                if(ext[1] == 'a')
                    return FILETYPE_YAI;
        }
    }
}