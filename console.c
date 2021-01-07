// Copyright (C) 2021 Brad Colbert

#include "readNetPBM.h"
#include "graphics.h"
#include "console.h"
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
extern byte IMAGE_FILE_TYPE;

// Globals
byte console_state = CONSOLE_HIDDEN;
byte* line = (byte*)CONSOLE_MEM;
char* tokens[] = { 0x0, 0x0, 0x0, 0x0, 0x0 };  // Maximum of 5 tokens
byte done = FALSE;

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
        if(line[i] != 0x0)
            return i;
    }

    return MAX_LINE_LEN;
}

byte skip_text(byte p)
{
    byte i;
    for(i = p; i < MAX_LINE_LEN; ++i)
    {
        if(line[i] == 0x0)
            return i;
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
            if(count < 5)
            {
                tokens[count++] = &line[x];
                x = skip_text(x);
            }
        }
        else
            break;
    }

    return count;
}

void process_command(byte ntokens)
{
    if(!ntokens)
        return;

    if(strncmp(tokens[0], "quit", MAX_LINE_LEN) == 0)
    {
        done = TRUE;

        return;
    }

    if(strncmp(tokens[0], "load", MAX_LINE_LEN) == 0)
    {
        if(ntokens > 1)
        {
            char filename[32];

            strncpy(filename, tokens[1], 32);

            image_file_type(filename);
            IMAGE_FILE_TYPE = FILETYPE_PGM; 
            if(IMAGE_FILE_TYPE)
            {
                int fd = open(filename, O_RDONLY);
                if(fd >= 0)
                {
                    switch(IMAGE_FILE_TYPE)
                    {
                        case FILETYPE_PBM:
                            disable_console();
                            set_graphics(GRAPHICS_8);
                            readPBMIntoGfx8(fd, (void*)MY_SCRN_MEM);
                            break;
                        case FILETYPE_PGM:
                            disable_console();
                            set_graphics(GRAPHICS_9);
                            readPGMIntoGfx9(fd, (void*)(MY_SCRN_MEM_TEMP), (void*)MY_SCRN_MEM);
                            break;
                    };
                    close(fd);
                }
            }
            else
            {
                gotoxy(0,0);
                cprintf(" ERROR: not filetype");
            }
            
  
            #if 0
            byte status = read_image_file(tokens[1], MY_SCRN_MEM_TEMP, MY_SCRN_MEM);
            if(!status)
            {
                gotoxy(0,0);
                memcpy(&line[40], line, 40);
                cprintf(" ERROR: Unable to load %s", tokens[1]);
                gotoxy(0,0);
            }

            else
            {
                /*
                // Setup graphics mode
                switch(status)
                {
                    case FILETYPE_PBM:
                        set_graphics(GRAPHICS_8);
                        break;
                    case FILETYPE_PGM:
                        set_graphics(GRAPHICS_9);
                        break;
                }
                */
            }
            #endif
        }
    }
}

void console_update(void)
{
    while(!done)
    {
        byte input = cgetc();
        byte x = wherex();

        // Handle command
        switch(input)
        {
            case CH_ENTER:
            {
                // process the tokens
                byte i;
                byte ntokens = 0;

                line[x] = 0x00;
                ntokens = get_tokens(x + 1);

                #ifdef DEBUG_CONSOLE
                // For debugging purposes, should display below the input
                // line which will not be shown when the system is active.
                memcpy(&line[40], line, 40);

                gotoxy(0,2);
                for(i = 0; i < x; i++)
                    cprintf("%02x ", line[i]);

                gotoxy(0,3);
                cprintf("%d ", ntokens);
                for(i = 0; i < ntokens; i++)
                {
                    cprintf("%d: %s\n", i, tokens[i]);
                }
                #endif

                if(ntokens)
                    process_command(ntokens);

                memset(line, 0, 40); // whipe the input line
                gotoxy(0,0);         // start at the begining of the input line
            }
            break;

            case CH_DEL:
                if(x > 0)
                {
                    line[x-1] = 0x0;
                    gotox(x-1);
                }
            break;

            default:
                cputc(input);
        }
    }
}