#ifndef _VBVXE_H_
#define _VBVXE_H_

#include <stdint.h>

// VBXE main registers 
// some of the registers can be r/w  
struct __vbxe {
    union {
        uint8_t VIDEO_CONTROL; // write
        uint8_t CORE_VERSION;  // read
    };
    union {
        uint8_t XDL_ADR0;      // write
        uint8_t MINOR_BERSION; // read
    };
    uint8_t XDL_ADR1; // write
    uint8_t XDL_ADR2; // write
    uint8_t CSEL;     // write
    uint8_t PSEL;     // write
    uint8_t CR;       // write
    uint8_t CG;       // write
    uint8_t CB;       // write
    uint8_t COLMASK;  // write
    union {
        uint8_t COLCLR;    // write
        uint8_t COLDETECT; // read
    };
    uint8_t reserved1[5]; // 4B-4F skipped
    union {
        uint8_t BL_ADR0;            // write
        uint8_t BLT_COLLISION_CODE; // read
    };
    uint8_t BL_ADR1; // write
    uint8_t BL_ADR2; // write
    union {
        uint8_t BLITTER_START; // write
        uint8_t BLITTER_BUSY;  // read
    };
    union {
        uint8_t IRQ_CONTROL; // write
        uint8_t IRQ_STATUS;  // read
    };
    uint8_t P0; // write
    uint8_t P1; // write
    uint8_t P2; // write
    uint8_t P3; // write
    uint8_t reserved2[4]; // 59-5C skipped
    uint8_t MEMAC_B_CONTROL; // write
    uint8_t MEMAC_CTRL;      // write & read
    uint8_t MEM_BANK_SEL;    // write & read
};

#define VBXE_D640 (*(struct __vbxe*)0xD640)
#define VBXE_D740 (*(struct __vbxe*)0xD740)
#define XDL ((uint8_t*)0x8000)

void setup_VBXE(void);
void clear_vbxe();

#endif