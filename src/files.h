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
void saveFile(const char filename[]);

#endif // FILES_H