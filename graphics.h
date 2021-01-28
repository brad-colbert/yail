// Copyright (C) 2021 Brad Colbert

#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "types.h"
#include "utility.h"

#include <stddef.h>

//
#define GRAPHICS_0 0x01
#define GRAPHICS_8 0x02
#define GRAPHICS_9 0x04
#define GRAPHICS_10 0x08
#define GRAPHICS_11 0x10
#define GRAPHICS_CONSOLE_EN 0x80
#define GRAPHICS_8_CONSOLE GRAPHICS_8 | GRAPHICS_CONSOLE_EN
#define GRAPHICS_9_CONSOLE GRAPHICS_9 | GRAPHICS_CONSOLE_EN

// Memory usage, per line
#define GFX_0_MEM_LINE 40
#define GFX_8_MEM_LINE 40
#define GFX_9_MEM_LINE 40
#define GFX_10_MEM_LINE 40
#define GFX_11_MEM_LINE 40

// Lines per mode
#define GFX_0_LINES 26
#define GFX_8_LINES 220
#define GFX_9_LINES 220
#define GFX_10_LINES 220
#define GFX_11_LINES 220

//
//#define CONSOLE_MEM 0xBC40  // We should read this from the system before we switch

// A simple structure for defining a display list in a code compact way
typedef struct _DLModeDef
{
    byte blank_lines;
    byte mode;        // From the Antic modes
    byte lines;       // # of lines of the mode
    byte dli;         // Switch for DLI
    void* buffer;     // Address of screen memory for mode, 0x0000 if use SAVMSC + offset
} DLModeDef;

typedef DLModeDef** DLModeDefParray;

#define MAX_MODE_DEFS 8
typedef struct _DLDef
{
    void* address;         // location of the DL
    size_t size;           // size in memory of the display list
    DLModeDef modes[MAX_MODE_DEFS]; // compact definition of the display list
} DLDef;

typedef struct _GfxDef
{
    byte mode;
    MemSegs buffer;
    DLDef dl; 
} GfxDef;

void saveCurrentGraphicsState(void);
void restoreGraphicsState(void);

void makeDisplayList(byte mode, const MemSegs* buffInfo, DLDef* dlInfo);
void makeGraphicsDef(byte mode, GfxDef* gfxInfo);

void setGraphicsMode(byte mode, byte keep);

void clearFrameBuffer(void);

void enableConsole(void);
void disableConsole(void);

void printDList(const char* name, DLDef* dlInfo);

#if 0
// Prototypes
void set_graphics(byte mode);
void set_graphics_console(byte enable);
void graphics_clear(void);
#endif

#endif // GRAPHICS_H