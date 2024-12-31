#ifndef _XLDC_H_
#define _XLDC_H_

// XDLC controls definition

//-----------------------------------------
// first byte 
#define XDLC_TMON   0x01 // 00000001 // Overlay text mode
#define XDLC_GMON   0x02 // 00000010 // Overlay graphics mode
#define XDLC_OVOFF  0x04 // 00000100 // disable overlay  
#define XDLC_MAPON  0x08 // 00001000 // enable color attributes
#define XDLC_MAPOFF 0x10 // 00010000 // disavble color attributes
#define XDLC_RPTL   0x20 // 00100000 // repeat for the next x scanlines
#define XDLC_OVADR  0x40 // 01000000 // set the address of the Overlay display memory (screen memory) and the step of the overlay display (how many pixels per line) 
#define XDLC_OVSCRL 0x80 // 10000000 // set scrolling values for the text mode
//-----------------------------------------
// second byte 
#define XDLC_CHBASE 0x01 // 00000001 // sets the font (text mode)
#define XDLC_MAPADR 0x02 // 00000010 // sets the address and step of the colour attribute map
#define XDLC_MAPPAR 0x04 // 00000100 // sets the scrolling values, width and height of a field in the colour attribute map
#define XDLC_ATT    0x08 // 00001000 // sets the display size(Narrow=256 pixels, Normal=320 pixels, Wide = 336 pixels) the Overlay priority to the ANTIC display and the Overlay color modification
#define XDLC_HR     0x10 // 00010000 // enables the high resolution mode, works only with graphics mode, 640 pixels with 16 colors supported 
#define XDLC_LR     0x20 // 00100000 // enables the low resolution mode, works only with graphics mode, 160 pixels with 256 colors supported 
// bit 6 in second byte is not in use (reserved)
#define XDLC_END    0x80 // 10000000 // ends the XDL and wait for VSYNC to occur


//-----------------------------------------
// XDLC XDLC_ATT first byte attributes
// OV_WIDTH
#define XDLC_ATT_OV_WIDTH_NARROW 0x00 // 00000000 // bit 0,1=00(256 pixels)
#define XDLC_ATT_OV_WIDTH_NORMAL 0x01 // 00000001 // bit 0,1=01(320 pixels)
#define XDLC_ATT_OV_WIDTH_WIDE   0x02 // 00000010 // bit 0,1=10(336 pixels)
// XDL OV PALETTE
#define XDLC_ATT_OV_PALETTE_00   0x00 // 00000000 // bit 4,5=00
#define XDLC_ATT_OV_PALETTE_01   0x10 // 00010000 // bit 4,5=01
#define XDLC_ATT_OV_PALETTE_10   0x20 // 00100000 // bit 4,5=10
#define XDLC_ATT_OV_PALETTE_11   0x30 // 00110000 // bit 4,5=11
// XDL PF PALETTE
#define XDLC_ATT_PF_PALETTE_00   0x00 // 00000000 // bit 6,7=00
#define XDLC_ATT_PF_PALETTE_01   0x40 // 01000000 // bit 6,7=01
#define XDLC_ATT_PF_PALETTE_10   0x80 // 10000000 // bit 6,7=10
#define XDLC_ATT_PF_PALETTE_11   0xC0 // 11000000 // bit 6,7=11
//-----------------------------------------
// XDLC XDLC_ATT second byte attributes
// MAIN_PRIORITY
#define XDLC_ATT_MAIN_PRIORITY_OVERLAY_PM0      0x01 // 00000001 
#define XDLC_ATT_MAIN_PRIORITY_OVERLAY_PM1      0x02 // 00000010 
#define XDLC_ATT_MAIN_PRIORITY_OVERLAY_PM2      0x04 // 00000100 
#define XDLC_ATT_MAIN_PRIORITY_OVERLAY_PM3      0x08 // 00001000 
#define XDLC_ATT_MAIN_PRIORITY_OVERLAY_PF0      0x10 // 00010000 
#define XDLC_ATT_MAIN_PRIORITY_OVERLAY_PF1      0x20 // 00100000 
#define XDLC_ATT_MAIN_PRIORITY_OVERLAY_PF2_PF3  0x40 // 01000000 
#define XDLC_ATT_MAIN_PRIORITY_OVERLAY_COLBK    0x80 // 10000000 
#define XDLC_ATT_MAIN_PRIORITY_OVERLAY_ALL      0xFF // 11111111%

#endif // _XLDC_H_