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
#define IMAGE_DL 0x6D00
#define COMMAND_DL_G8 0x6E00
#define COMMAND_DL_G9 0x6F00
#define MY_SCRN_MEM 0x7000 // 1024 byte aligned
#define MY_SCRN_MEM_B 0x8000
#define MY_SCRN_MEM_C 0x9000
#define CONSOLE_MEM 0xCC40
#ifdef GR_8
#define FILENAME "MTFUJID.PBM"
#else
#define FILENAME "MTFUJI.PGM"
#endif

// Globals
int fd;
#ifdef GR_8
#else
byte ORG_GFX_STATE = 0x00;

void disable_9(void);

// Enable Gfx 9
#pragma optimize(push, off)
void enable_9(void) {
    __asm__("pha");
    POKE(PRIOR, ORG_GFX_STATE | 0x40);
    POKEW(VDSLST, (unsigned)disable_9);
    POKE(WSYNC, 255);
    __asm__("pla");
    __asm__("rti");
    /*
    __asm__("pha");
    __asm__("lda $40");
    __asm__("sta $26F");
    // Disable Gfx9
    __asm__("lda $%v", disable_9);
    __asm__("sta $200");
    __asm__("lda $%v", ((byte*)disable_9+1));
    __asm__("sta $201");
    __asm__("pla");
    __asm__("rti");
    */
}
#pragma optimize(pop)

// Disable Gfx 9
#pragma optimize(push, off)
void disable_9(void) {
    __asm__("pha");
    POKE(PRIOR, ORG_GFX_STATE);
    POKEW(VDSLST, (unsigned)enable_9);
    POKE(WSYNC, 255);
    __asm__("pla");
    __asm__("rti");
    /*
    __asm__("pha");
    __asm__("lda %v", ORG_GFX_STATE);
    __asm__("sta $26F");
    // Re-enable Gfx9
    __asm__("lda $%v", enable_9);
    __asm__("sta $200");
    __asm__("lda $%v", enable_9+1);
    __asm__("sta $201");
    __asm__("pla");
    __asm__("rti");
    */
}
#pragma optimize(pop)
#endif

//
int main()
{
    //char input;
    unsigned ORG_DLIST = PEEKW(SDLSTL);
    byte NMI_STATE = PEEK(NMIEN);
    // 320x220
    struct dl_def image_dl[] = { {8, DL_MAP320x1x1, 102, MY_SCRN_MEM, 0},
                                 {0, DL_MAP320x1x1, 102, MY_SCRN_MEM_B, 0},
                                 {0, DL_MAP320x1x1, 16, MY_SCRN_MEM_C, 0}
                               };
    struct dl_def command_dl_g8[] = { {8, DL_MAP320x1x1, 102, MY_SCRN_MEM, 0},
                                      {0, DL_MAP320x1x1, 102, MY_SCRN_MEM_B, 0},
                                      {0, DL_MAP320x1x1, 8, MY_SCRN_MEM_C, 0},
                                      {0, DL_CHR40x8x1, 1, CONSOLE_MEM, 0}
                                    };
    struct dl_def command_dl_g9[] = { {8, DL_MAP320x1x1, 102, MY_SCRN_MEM, 0},
                                      {0, DL_MAP320x1x1, 102, MY_SCRN_MEM_B, 0},
                                      {0, DL_MAP320x1x1, 7, MY_SCRN_MEM_C, 0},
                                      {0, DL_MAP320x1x1, 1, NULL, 1},
                                      {0, DL_CHR40x8x1, 1, CONSOLE_MEM, 1}
                                    };

    // Generate our display lists.
    makeDisplayList((void*)IMAGE_DL, image_dl, 3);
    makeDisplayList((void*)COMMAND_DL_G8, command_dl_g8, 4);
    makeDisplayList((void*)COMMAND_DL_G9, command_dl_g9, 5);

    fd = open(FILENAME, O_RDONLY);
    if(fd >= 0)
    {
        // Clear the text
        clrscr();

        // Clear graphics memory
        memset((void*)MY_SCRN_MEM, 0x00, 0x3000);

        // Read the data into the framebuffer
        #ifdef LOAD_FIRST
        #ifdef GR_8
        readPBMIntoGfx8(fd, (void*)MY_SCRN_MEM);
        #else
        readPGMIntoGfx9(fd, (void*)(MY_SCRN_MEM_C + 0x0400), (void*)MY_SCRN_MEM);
        #endif
        #endif

        // Enable graphics modes
        #ifdef GR_8
        POKE(COLOR1, 255);
        POKE(COLOR2, 0);
        POKEW(SDLSTL, COMMAND_DL_G8);          // Tell ANTIC the address of our display list (use it)
        #else
        ORG_GFX_STATE = PEEK(GPRIOR);       // Save current priority states
        POKE(GPRIOR, ORG_GFX_STATE | 64);   // Enable GTIA   
        POKE(COLOR2,0x00);                  // Turn the console black
        POKEW(SDLSTL, IMAGE_DL);            // Tell ANTIC the address of our display list (use it)
        #endif

        // Load data
        #ifndef LOAD_FIRST
        #ifdef GR_8
        readPBMIntoGfx8(fd, (void*)MY_SCRN_MEM);
        #else
        readPGMIntoGfx9(fd, (void*)(MY_SCRN_MEM_C + 0x0400), (void*)MY_SCRN_MEM);

        POKEW(SDLSTL, COMMAND_DL_G9);       // Tell ANTIC the address of our display list (use it)
        POKEW(VDSLST, (unsigned)disable_9); // Set the address to our DLI that disables GTIA
        POKE(NMIEN, NMI_STATE | 128);       // Enable NMI
        #endif
        #endif

        //printf("Hit <Return> to view");
        gotoxy(0,0);
        cputs("Hit <Return> to continue...");
        cgetc();
    }
    else
    {
        printf("Unable to open the file %s\n", FILENAME);
    }
    
    /*
    clrscr();
    gotoxy(0,0);
    cursor(1);
    input = cgetc();  // hit key to quit
    while(input != CH_ESC)
    {
        cputc(input);
        input = cgetc();
    }
    */

    // Return the graphics modes
    #ifdef GR_8
    #else
    POKE(NMIEN, NMI_STATE);      // Restore the previous NMI state
    POKE(GPRIOR, ORG_GFX_STATE); // Restore the previous priority states
    #endif
    POKEW(SDLSTL, ORG_DLIST);    // Tell ANTIC to use the original display list

    return 0;
}