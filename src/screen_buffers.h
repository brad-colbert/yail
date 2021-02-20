// Copyright (C) 2021 Brad Colbert

#ifndef SCREEN_BUFFERS_H
#define SCREEN_BUFFERS_H

#include "types.h"

#include <unistd.h>

#define BUFF_SIZE 0x22B0
#define BUFF_BOUNDARIES = BUFF_SIZE / 0x1000

#pragma bss-name (push,"SCREEN_R")  
extern byte screen_red[]; 
#pragma bss-name (pop)
#pragma bss-name (push,"SCREEN_G")  
extern byte screen_green[]; 
#pragma bss-name (pop)
#pragma bss-name (push,"SCREEN_B")  
extern byte screen_blue[]; 
#pragma bss-name (pop)

#pragma bss-name (push,"SCREEN_R_SEGS")  
extern size_t red_buff_segs[]; 
#pragma bss-name (pop)
#pragma bss-name (push,"SCREEN_G_SEGS")  
extern size_t green_buff_segs[]; 
#pragma bss-name (pop)
#pragma bss-name (push,"SCREEN_B_SEGS")  
extern size_t blue_buff_segs[]; 
#pragma bss-name (pop)

#pragma bss-name (push,"SCREEN_R_SIZES")  
extern size_t red_buff_sizes[]; 
#pragma bss-name (pop)
#pragma bss-name (push,"SCREEN_G_SIZES")  
extern size_t green_buff_sizes[]; 
#pragma bss-name (pop)
#pragma bss-name (push,"SCREEN_B_SIZES")  
extern size_t blue_buff_sizes[]; 
#pragma bss-name (pop)

#endif // SCREEN_BUFFERS_H