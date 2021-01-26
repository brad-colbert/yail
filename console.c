// Copyright (C) 2021 Brad Colbert

#include "readNetPBM.h"
#include "graphics.h"
#include "console.h"
#include "files.h"
#include "consts.h"
#include "types.h"

#include <atari.h>
#include <conio.h>

#include <fcntl.h>
#include <stdio.h>
#include <string.h>

// Defines
#define MAX_LINE_LEN 40

// Externs
extern byte GRAPHICS_MODE;
extern void* MY_SCRN_MEM;
extern void* MY_SCRN_MEM_TEMP;
extern byte IMAGE_FILE_TYPE;
extern struct dl_store image_dl_store;


// Globals
byte console_state = CONSOLE_HIDDEN;
char* console_mem = (byte*)CONSOLE_MEM;
char line[MAX_LINE_LEN+1];
char* tokens[] = { 0x0, 0x0, 0x0, 0x0, 0x0 };  // Maximum of 5 tokens
byte done = FALSE;

void reset_console(void)
{
    memset(line, 0, 40);           // wipe the input line
    memset(console_mem, 0, 40*24); // wipe all of the console mem
    gotoxy(0,0);                   // start at the begining of the input line
}

void enable_console(void)
{
    console_state = CONSOLE_SHOWN;
}

void disable_console(void)
{
    console_state = CONSOLE_HIDDEN;
}

byte skip_empty(byte p)
{
    byte i;
    for(i = p; i < MAX_LINE_LEN; ++i)
    {
        if(line[i] != ' ')
            return i;
        if(line[i] == 0x0)
            return MAX_LINE_LEN;
    }

    return MAX_LINE_LEN;
}

byte skip_text(byte p)
{
    byte i;
    for(i = p; i < MAX_LINE_LEN; ++i)
    {
        if(line[i] == ' ')
            return i;
        if(line[i] == 0x0)
            return MAX_LINE_LEN;
    }

    return MAX_LINE_LEN;
}

byte get_tokens(byte endx)
{
    byte count = 0;
    byte x = 0;

    while(1)
    {
        x = skip_empty(x);

        if(x < endx)
        {
            if(count < 3)
            {
                tokens[count++] = &line[x];
                x = skip_text(x);
            }
        }
        else
            break;
    }

    // Replace spaces with nulls
    for(x = 0; (x < MAX_LINE_LEN) || (line[x] == 0x0) ; ++x)
        if(line[x] == ' ')
            line[x] = 0x0;

    return count;
}

void fix_chars(char* buff)
{
    byte i;
    for(i=0;i<MAX_LINE_LEN;++i)
        if(buff[i] == 0x0E)   // Change _ to .
            buff[i] = 0x2E;
}

void process_command(byte ntokens)
{
    if(!ntokens)
        return;

    if(strncmp(tokens[0], "help", 4) == 0)
    {
        const char help[] =
        "help - This screan\n\r"
        "quit - Exit this utility\n\r"
        "cls  - Clear the image display\n\r"
        "gfx  - [0,8,9] Set the graphics mode\n\r"
        "load - [filename] Load and display file\n\r"
        "save - [filename] Save memory to YAI\n\r"
        "\n\r"
        "Any key to continue...\n\r";

        byte last_graphics_mode = GRAPHICS_MODE;  // Save current graphics mode
        set_graphics(GRAPHICS_0);                 // Switch to text mode
        gotoxy(0,0);                              // Start at the console origin
        cputs(help);                              // Show the help text
        cgetc();                                  // Wait
        set_graphics(last_graphics_mode);         // Switch back to starting graphics mode
    }

    if(strncmp(tokens[0], "quit", 4) == 0)
    {
        done = TRUE;

        return;
    }

    if(strncmp(tokens[0], "gfx", 3) == 0)
    {
        if(ntokens > 1)
        {
            switch(tokens[1][0])
            {
                case '0':
                    set_graphics(GRAPHICS_0);
                    break;

                case '8':
                    set_graphics(GRAPHICS_8);
                    break;

                case '9':
                    set_graphics(GRAPHICS_9);
                    break;

                case '1':
                    switch(tokens[1][1])
                    {   case '0':
                            set_graphics(GRAPHICS_10);
                            break;
                        case '1':
                            set_graphics(GRAPHICS_11);
                            break;
                    }
                    break;
            }
        }
    }

    if(strncmp(tokens[0], "cls", 3) == 0)
    {
        graphics_clear();
    }

    if(strncmp(tokens[0], "load", 4) == 0)
    {
        if(ntokens > 1)
        {
            struct dl_store dl_mem[MAX_N_DL];
            struct dli_store dli_mem[MAX_N_DLI];
            struct mem_store gfx_mem[MAX_N_MEM];

            fix_chars(tokens[1]);
            load_file(tokens[1], &GRAPHICS_MODE, dl_mem, dli_mem, gfx_mem, 1);
        }
        else
        {
            gotoxy(0,0);
            cprintf("ERROR: File not specified");
        }
    }

    if(strncmp(tokens[0], "save", 4) == 0)
    {
        if(ntokens > 1)
        {
            struct dl_store dl_mem[MAX_N_DL];
            struct dli_store dli_mem[MAX_N_DLI];
            struct mem_store gfx_mem[MAX_N_MEM];

            memset(&dl_mem, 0, sizeof(dl_mem));
            memset(&dli_mem, 0, sizeof(dli_mem));
            memset(&gfx_mem, 0, sizeof(gfx_mem));

            // Define the DL description
            dl_mem[0] = image_dl_store;

            // Define the DLI description

            // Define the MEM descriptions
            gfx_mem[0].size = 0x2800;
            gfx_mem[0].mem = (void*)MY_SCRN_MEM;

            save_file(tokens[1], dl_mem, dli_mem, gfx_mem);
        }
        else
        {
            gotoxy(0,0);
            cprintf("ERROR: File not specified");
        }
    }
}

//#define DEBUG_CONSOLE

void console_update(void)
{
    while(!done)
    {
        byte input = cgetc();
        byte x = wherex();

        // Handle quit
        if(input == CH_ESC)
        {
            #ifdef DEBUG_CONSOLE
            cputs("ESC hit... quiting\n\r");
            cgetc();
            #endif

            set_graphics(GRAPHICS_0);
            return;
        }

        // Control the display of the console:
        // Keypress, if the console is not up, bring it up.
        // Enter, not up, bring it up.  Up, bring it down.
        // Handle command
        switch(input)
        {
            case CH_ENTER:
                #ifdef DEBUG_CONSOLE
                cputs("ENTER hit... toggle console\n\r");
                cgetc();
                #endif

                console_state = !console_state;
                set_graphics(GRAPHICS_MODE);
                break;

            default:
                if(!console_state)
                {
                    enable_console();
                    set_graphics(GRAPHICS_MODE);
                }
                break;
        }
    
        // Handle command
        switch(input)
        {
            case CH_ENTER:
            {
                // process the tokens
                byte ntokens = 0;

                ntokens = get_tokens(x + 1);

                #ifdef DEBUG_CONSOLE
                // For debugging purposes, should display below the input
                // line which will not be shown when the system is active.
                {
                    byte i;

                    //memcpy(&line[40], line, 40);

                    cclearxy(0, 4, 40);
                    gotoxy(0,4);
                    for(i = 0; i < x; i++)
                        cprintf("%02x ", line[i]);

                    gotoxy(0,5);
                    cprintf("%d\n\r", ntokens);
                    for(i = 0; i < ntokens; i++)
                    {
                        cclearxy(0, i+5, 40);
                        gotoxy(0,i+5);
                        cprintf("%d: %s", i, tokens[i]);
                    }
                }
                #endif

                if(ntokens)
                    process_command(ntokens);

                reset_console();
            }
            break;

            case CH_DEL:
                if(x > 0)
                {
                    line[x-1] = 0x0;
                    console_mem[x-1] = 0x0;
                    gotox(x-1);
                }
            break;

            default:
                line[x] = input;
                line[x+1] = 0x0;
                #ifdef DEBUG_CONSOLE
                {
                    byte i;
                    for(i=0; i<x; ++i)
                    {
                        gotoxy(i*2, 1);
                        cprintf("%02x ", line[i]);
                    }
                    gotoxy(x, 0);
                }
                #endif
                cputc(input);
        }
    }
}