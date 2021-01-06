#include "graphics.h"
#include "console.h"
#include "displaylist.h"
#include "consts.h"
#include "types.h"

#include <atari.h>
#include <peekpoke.h>
#include <string.h>
#include <stdio.h>

// Globals (private)
byte ORG_GFX_STATE = 0x00;
unsigned ORG_DLIST;
byte NMI_STATE;
byte ORG_COLOR1, ORG_COLOR2;
//unsigned CONSOLE_MEM = 0xFFFF;

// Display list definitions
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
                                  {0, DL_MAP320x1x1, 1, 0x0, 1},
                                  {0, DL_CHR40x8x1, 1, CONSOLE_MEM, 1}
                                };

// Externals
extern byte console_state;

// DLI definitions
void disable_9_dli(void);  // prototype for below

// Enable Gfx 9
#pragma optimize(push, off)
void enable_9_dli(void) {
    __asm__("pha");
    POKE(PRIOR, ORG_GFX_STATE | 0x40);
    POKEW(VDSLST, (unsigned)disable_9_dli);
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
void disable_9_dli(void) {
    __asm__("pha");
    POKE(PRIOR, ORG_GFX_STATE);
    POKEW(VDSLST, (unsigned)enable_9_dli);
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

void save_current_graphics_state(void)
{
    unsigned cmem;

    ORG_DLIST = PEEKW(SDLSTL);
    NMI_STATE = PEEK(NMIEN);
    ORG_GFX_STATE = PEEK(GPRIOR);       // Save current priority states
    ORG_COLOR1 = PEEK(COLOR1);
    ORG_COLOR2 = PEEK(COLOR2);
    //CONSOLE_MEM = PEEKW(SAVMSC);

    cmem = PEEKW(SAVMSC);
    printf("C: %p\n", cmem);
    print_dlist("Original: ", ORG_DLIST);
}

void restore_graphics_state(void)
{
    POKE(COLOR2, ORG_COLOR2);
    POKE(COLOR1, ORG_COLOR1);
    POKE(GPRIOR, ORG_GFX_STATE);       // Save current priority states
    POKE(NMIEN, NMI_STATE);
    POKEW(SDLSTL, ORG_DLIST);
}

void set_graphics(byte mode)
{
    if(console_state)
    {
        switch(mode)
        {
            case GRAPHICS_8:
                makeDisplayList((void*)COMMAND_DL_G8, command_dl_g8, 4);
                POKE(COLOR1, 255);
                POKE(COLOR2, 0);
                POKEW(SDLSTL, COMMAND_DL_G8);          // Tell ANTIC the address of our display list (use it)
            break;
            case GRAPHICS_9:
                makeDisplayList((void*)COMMAND_DL_G9, command_dl_g9, 5);
                POKE(COLOR2, 0);                        // Turn the console black
                POKE(GPRIOR, ORG_GFX_STATE | 64);       // Enable GTIA   
                POKEW(SDLSTL, COMMAND_DL_G9);           // Tell ANTIC the address of our display list (use it)
                POKEW(VDSLST, (unsigned)disable_9_dli); // Set the address to our DLI that disables GTIA for the console
                POKE(NMIEN, NMI_STATE | 128);           // Enable NMI
            break;
        }
    }
    else
    {
        // Build the display list
        switch(mode)
        {
            case GRAPHICS_8:
            case GRAPHICS_9:
                makeDisplayList((void*)IML_DL, image_dl, 3);
            break;
        }

        // Set graphics mode specifc things
        switch(mode)
        {
            case GRAPHICS_8:
                POKE(COLOR1, 255);
                POKE(COLOR2, 0);
            break;
            case GRAPHICS_9:
                POKE(GPRIOR, ORG_GFX_STATE | 64);   // Enable GTIA   
            break;
        }

        // Enable the graphics mode
        switch(mode)
        {
            case GRAPHICS_8:
            case GRAPHICS_9:
                POKEW(SDLSTL, IMAGE_DL);            // Tell ANTIC the address of our display list (use it)
            break;
        }
    }
    
}
