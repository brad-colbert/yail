// Copyright (C) 2021 Brad Colbert

#ifndef FILES_H
#define FILES_H

#include "types.h"

#define FILETYPE_PBM 0x01
#define FILETYPE_PGM 0x02
#define FILETYPE_YAI 0x03

// Display list storage structure
struct dl_store
{
    unsigned size;
    void* mem;
};
#define MAX_N_DL 8
#define DL_TOKEN 0x01

// Display list interupt storage structure
struct dli_store
{
    unsigned size;
    void* mem;
};
#define MAX_N_DLI 8
#define DLI_TOKEN 0x02

// Screen memory storage structure
struct mem_store
{
    unsigned size;
    void* mem;
};
#define MAX_N_MEM 8
#define MEM_TOKEN 0x03

byte imageFileType(const char filename[]);
byte loadFile(const char filename[]);
#if 0
// filename - The name of the file to save the image.
// gfx_mode - The graphics mode to set when displaying the image.
// dl_mem - An array of pointers to the display lists
// dli_mem - An array of pointers to DLI code.
// gfx_mem - An array of graphics memory locations
// set_graphics_mode - If set to 1, set the gfx mode after loading it, else don't
void save_file(const char filename[],
               struct dl_store dl_mem[MAX_N_DL],
               struct dli_store dli_mem[MAX_N_DLI],
               struct mem_store gfx_mem[MAX_N_MEM]);
byte load_file(const char filename[],
               byte* gfx_mode,
               struct dl_store dl_mem[MAX_N_DL],
               struct dli_store dli_mem[MAX_N_DLI],
               struct mem_store gfx_mem[MAX_N_MEM],
               byte set_graphics_mode);

byte image_file_type(const char filename[]);
#endif

#endif // FILES_H