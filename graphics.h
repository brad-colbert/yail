// Copyright (C) 2021 Brad Colbert

#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "types.h"

//
#define GRAPHICS_0 0
#define GRAPHICS_8 1
#define GRAPHICS_9 2
#define GRAPHICS_CONSOLE_EN 0x10
#define GRAPHICS_8_CONSOLE GRAPHICS_8 | GRAPHICS_CONSOLE_EN
#define GRAPHICS_9_CONSOLE GRAPHICS_9 | GRAPHICS_CONSOLE_EN

// Display lists memory locations
#define IML_DL 0x7C00  // provides 1K before bumping into video memory

// Screen memory location (high resolution spaces 3 4K segments)
#define MY_SCRN_MEM 0x8000 // 1024 byte aligned
#define MY_SCRN_MEM_B 0x9000
#define MY_SCRN_MEM_C 0xA000
#define MY_SCRN_MEM_TEMP (MY_SCRN_MEM_C + 0x0400)

//
#define CONSOLE_MEM 0xBC40  // We should read this from the system before we switch

// Prototypes
void save_current_graphics_state(void);
void restore_graphics_state(void);
void set_graphics(byte mode);
void set_graphics_console(byte enable);
void graphics_clear(void);

#endif // GRAPHICS_H