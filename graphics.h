#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "types.h"

//
#define GRAPHICS_0 0
#define GRAPHICS_8 1
#define GRAPHICS_9 2

// Display lists memory locations
#define IMAGE_DL 0x6D00
#define COMMAND_DL_G8 0x6E00
#define COMMAND_DL_G9 0x6F00
#define IML_DL 0x6F00

// Screen memory location (high resolution spaces 3 4K segments)
#define MY_SCRN_MEM 0x7000 // 1024 byte aligned
#define MY_SCRN_MEM_B 0x8000
#define MY_SCRN_MEM_C 0x9000

//
#define CONSOLE_MEM 0xBC40  // We should read this from the system before we switch

// Prototypes
void save_current_graphics_state(void);
void restore_graphics_state(void);
void set_graphics(byte mode);


#endif // GRAPHICS_H