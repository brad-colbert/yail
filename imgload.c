// Copyright (C) 2021 Brad Colbert

#include "console.h"
#include "graphics.h"
#include "files.h"
#include "utility.h"
#include "displaylist.h"

#include <atari.h>
#include <conio.h>
#include <peekpoke.h>
#include <stdlib.h>
#include <string.h>

//
extern byte GRAPHICS_MODE;

//
int main(int argc, char* argv[])
{
    // //dl_def dl = {0, DL_CHR40x8x1, 1, 1, CONSOLE_MEM};
    // dl_def dl = {8, DL_MAP320x1x1, 220, 0, MY_SCRN_MEM};
    // dl_def_parray dlist = 0x0;
    // dl_def* dlist_ptr = 0x0;
    // byte i = 0;

    // reset_console();

    // dlist = expandDisplayList(&dl);
    // dlist_ptr = dlist[0];

    // while(dlist_ptr)
    // {
    //     cprintf("%d: %d, %d, %d, %d, %02X\n\r", i, dlist_ptr->blank_lines, dlist_ptr->mode, dlist_ptr->lines, dlist_ptr->dli, dlist_ptr->address);
    //     ++i;
    //     dlist_ptr = dlist[i];
    // }

    // cleanupDL_Def_PArray(dlist);
    // struct dl_store image_dl_store = { 0, 0 };
    // dl_def dl[] = { {8, DL_MAP320x1x1, 211, 0, MY_SCRN_MEM},
    //                 {0, DL_MAP320x1x1, 1, 1, 0x0},
    //                 {0, DL_CHR40x8x1, 1, 1, CONSOLE_MEM}
    //               };
    // makeDisplayList(0xA000, dl, 3, &image_dl_store);
    // print_dlist("DL", image_dl_store.mem);
    // cgetc();


    #if 0
    void* testmem[26] = { 0x0, 0x0, 0x0 };
    unsigned int memlo = PEEKW(743);
    byte i = 0;
    #endif

    // Clear the text and memory
    enable_console();
    cursor(1);
    reset_console();

    save_current_graphics_state();
    
    graphics_clear();


    #if 0
    cprintf("MEMLO %04X\r\n", memlo);

    for(;i<26;++i)
    {
        testmem[i] = malloc(1024);
        cprintf("buffloc(%d) = %p\r\n", i, testmem[i]);
    }
    //testmem[1] = malloc(40*220);
    //cprintf("buffloc = %p\r\n", testmem[1]);
    //testmem[2] = malloc(40*220);
    //cprintf("buffloc = %p\r\n", testmem[2]);
    for(i = 0;i<26;++i)
        free(testmem[i]);
    //free(testmem[1]);
    //free(testmem[2]);

    cgetc();
    /*
    return 0;
    
    */
   #endif



    //memset(0x6000, 0x55, 0x6FFF);
    set_graphics(GRAPHICS_8);  // Start in Gfx 8 for giggles

    if(argc > 1)
    {
        struct dl_store dl_mem[MAX_N_DL];
        struct dli_store dli_mem[MAX_N_DLI];
        struct mem_store gfx_mem[MAX_N_MEM];

        fix_chars(argv[1]);
        load_file(argv[1], &GRAPHICS_MODE, dl_mem, dli_mem, gfx_mem, 1);
    }
    
    console_update();

    restore_graphics_state();

    return 0;
}