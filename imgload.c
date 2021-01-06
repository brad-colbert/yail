#include <conio.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#include "readNetPBM.h"
#include "console.h"
#include "graphics.h"
#include "displaylist.h"
#include "types.h"
#include "consts.h"

#define GR_9

#ifdef GR_8
#define FILENAME "MTFUJID.PBM"
#else
#define FILENAME "MTFUJI.PGM"
#endif

// Globals
int fd;

// Externs
extern byte ORG_GFX_STATE;
extern unsigned ORG_DLIST;
extern byte NMI_STATE;

//
int main()
{
    fd = open(FILENAME, O_RDONLY);
    if(fd >= 0)
    {
        // Clear the text and memory
        clrscr();
        memset((void*)MY_SCRN_MEM, 0x00, 0x3000);

        save_current_graphics_state();

        cgetc();

        #ifdef GR_8
        set_graphics(GRAPHICS_8);

        readPBMIntoGfx8(fd, (void*)MY_SCRN_MEM);

        enable_console();
        set_graphics(GRAPHICS_8);
        #else
        set_graphics(GRAPHICS_9);

        readPGMIntoGfx9(fd, (void*)(MY_SCRN_MEM_C + 0x0400), (void*)MY_SCRN_MEM);

        enable_console();
        set_graphics(GRAPHICS_9);
        #endif

        //printf("Hit <Return> to view");
        gotoxy(0,0);
        cputs("Hit <Return> to continue...\n\r");
        cgetc();

        restore_graphics_state();
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

    // // Return the graphics modes
    // #ifdef GR_8
    // #else
    // POKE(NMIEN, NMI_STATE);      // Restore the previous NMI state
    // POKE(GPRIOR, ORG_GFX_STATE); // Restore the previous priority states
    // #endif
    // POKEW(SDLSTL, ORG_DLIST);    // Tell ANTIC to use the original display list

    return 0;
}