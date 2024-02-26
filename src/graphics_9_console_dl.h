#ifndef GRAPHICS_9_DL_H
#define GRAPHICS_9_DL_H

#include <atari.h>

void graphics_9_console_dl = {
    DL_DLI(DL_BLK8),                                                 // DLI disables the graphics 9 state (GTIA)
    DL_LMS(DL_GRAPHICS0), 0xFFFF,                                    // Header that shows application name and version
    DL_DLI(DL_BLK1),                                                 // DLI enables the graphics 9 state (GTIA)
    DL_LMS(DL_GRAPHICS9), &framebuffer[0]+360, 
    DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9,
    DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9,
    DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9,
    DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9,
    DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9,
    DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9,
    DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9,
    DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9,
    DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9,
    DL_GRAPHICS9, DL_GRAPHICS9,
    DL_LMS(DL_GRAPHICS9), &framebuffer[0]+0x1000, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9,
    DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9,
    DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9,
    DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9,
    DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9,
    DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9,
    DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9,
    DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9, DL_GRAPHICS9,
    // 5 text lines
    DL_GRAPHICS9, DL_DLI(DL_GRAPHICS9),                                                             // DLI disables the graphics 9 state (GTIA)
    DL_LMS(DL_GRAPHICS0), 0xFFFF, DL_GRAPHICS0, DL_GRAPHICS0, DL_GRAPHICS0, DL_DLI(DL_GRAPHICS0),   // DLI enables the graphics 9 state (GTIA)
    DL_JVB, &graphics_9_console_dl
};

#endif // GRAPHICS_9_DL