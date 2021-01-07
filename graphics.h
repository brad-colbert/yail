#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "types.h"

//
#define GRAPHICS_0 0
#define GRAPHICS_8 1
#define GRAPHICS_9 2

// Display lists memory locations
#define IML_DL 0x7000
#define GR8_DL 0xA400
#define GR9_DL 0xA800

// Screen memory location (high resolution spaces 3 4K segments)
#define MY_SCRN_MEM 0x8000 // 1024 byte aligned
#define MY_SCRN_MEM_B 0x9000
#define MY_SCRN_MEM_C 0xA000

//
#define CONSOLE_MEM 0xBC40  // We should read this from the system before we switch

// Prototypes
void save_current_graphics_state(void);
void restore_graphics_state(void);
void set_graphics(byte mode);

#endif // GRAPHICS_H