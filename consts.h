// Copyright (C) 2021 Brad Colbert

#ifndef CONSTS_H
#define CONSTS_H

// Some defines to make my life easier
#define SAVMSC 0x58  // Stores address for start of screen memory
#define VDSLST 0x200
#define SDMCTL 0x22F
#define SDLSTL 0x230
#define GPRIOR 0x26F
#define PRIOR 0xD01B
#define COLOR0 0x2C4
#define COLOR1 0x2C5
#define COLOR2 0x2C6
#define COLOR3 0x2C7
#define COLOR4 0x2C8
#define WSYNC 0xD40A
#define NMIEN 0xD40E

// GTIA graphics bitmasks
#define GFX_9 0x40
#define GFX_10 0x80
#define GFX_11 0xC0

// Important ASCII codes
#define ASC_TAB 0x09
#define ASC_LF 0x0A
#define ASC_CR 0x15
#define ASC_SPC 0x20
#define ASC_HASH 0x23
#define ASC_0 0x30

//
#define TRUE 0xFF
#define FALSE 0x00;

#endif // CONSTS_H
