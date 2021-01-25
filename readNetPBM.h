// Copyright (C) 2021 Brad Colbert

#ifndef READ_NETPBM_H
#define READ_NETPBM_H

#include "types.h"

#define READ_BYTE(FD, B) \
    if(!read(FD, &B, 1)) \
        return -1;

// Reads a file from fd and writes numbytes of it into dmem.
// Assumes destination will be Gfx8 formatted
void readPBMIntoGfx8(int fd, byte* dmem);

// Reads a file from fd and writes numbytes of it into dmem.
// Assumes destination will be Gfx9 formatted
void readPGMIntoGfx9(int fd, byte* tmem, byte* dmem);

//byte read_image_file(char* filename, byte* tmem, byte* dmem);

#endif // READ_NETPBM_H
