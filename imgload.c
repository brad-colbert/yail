#include <conio.h>
#include <atari.h>
#include <peekpoke.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include "readNetPBM.h"
#include "displaylist.h"
#include "types.h"
#include "consts.h"

//#define GR_8

// This organizes the display list before the screen memory
#define MY_DL 0x6C00
#define MY_SCRN_MEM (MY_DL + 0x0400) // 1024 byte aligned
#define MY_SCRN_MEM_B 0x8000
#define MY_SCRN_MEM_C 0x9000
#ifdef GR_8
#define FILENAME "MTFUJID.PBM"
#else
#define FILENAME "MTFUJI.PGM"
#endif

// Globals
int fd;

//
int main()
{
    unsigned ORG_DLIST = PEEKW(SDLSTL);
    #ifdef GR_8
    #else
    byte ORG_GFX_STATE = PEEK(GPRIOR);
    #endif
    // 320x220
    struct dl_def dls[] = { {8, DL_MAP320x1x1, 102, MY_SCRN_MEM},
                            {0, DL_MAP320x1x1, 102, MY_SCRN_MEM_B},
                            {0, DL_MAP320x1x1, 16, MY_SCRN_MEM_C} };

    // Generate a display list and show the values.
    makeDisplayList((void*)MY_DL, dls, 3);

    fd = open(FILENAME, O_RDONLY);
    if(fd >= 0)
    {
        // Read the data into the framebuffer
        memset((void*)MY_SCRN_MEM, 0x00, 0x3000);

        #ifdef GR_8
        readPBMIntoGfx8(fd, (void*)MY_SCRN_MEM);
        #else
        readPGMIntoGfx9(fd, (void*)(MY_SCRN_MEM_C + 0x0400), (void*)MY_SCRN_MEM);
        //readPGMIntoGfx9(fd, (void*)0xA000, (void*)MY_SCRN_MEM);
        #endif

        printf("Hit <Return> to view");
        cgetc();

        #ifdef GR_8
        POKE(COLOR1, 0);
        POKE(COLOR2, 15);
        #else
        POKE(GPRIOR,ORG_GFX_STATE | 64);
        #endif
        POKEW(SDLSTL, MY_DL);
    }
    else
    {
        printf("Unable to open the file %s\n", FILENAME);
    }

    cgetc();  // hit key to quit

    // Return the graphics modes
    #ifdef GR_8
    #else
    POKE(623, ORG_GFX_STATE);
    #endif
    POKEW(SDLSTL, ORG_DLIST);

    return 0;
}