// Copyright (C) 2021 Brad Colbert
#if 0

#include "netimage.h"
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
#define CONSOLE_LINES 5
#define CONSOLE_HIDDEN 0
#define CONSOLE_SHOWN 1
#define MAX_LINE_LEN 40

// Externs
extern byte IMAGE_FILE_TYPE;

// Globals
byte console_lines = CONSOLE_LINES;
byte console_state = CONSOLE_HIDDEN;
#ifdef CONSOLE_USE_LOCAL_BUFFER
char console_buff[GFX_0_MEM_LINE * CONSOLE_LINES];
#else
char* console_buff = 0x0;
#endif
char* tokens[] = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };  // Maximum of 8 tokens
byte done = FALSE;
char server[80] = { "N:TCP://192.168.1.205:9999/\"\0" };

void reset_console(void)
{
    memset(console_buff, 0, GFX_0_MEM_LINE * console_lines); // wipe all of the console mem
    gotoxy(0,0);                   // start at the begining of the input line
}

#define SEARCHING_FOR_TOKEN 1
#define START_OF_TOKEN 2
#define END_OF_TOKEN 3
byte get_tokens(byte* buff, byte endx)
{
    byte count = 0;
    byte state = SEARCHING_FOR_TOKEN;
    byte i;
    for(i = 0; i < endx; ++i)
    {
        switch(state)
        {
            case SEARCHING_FOR_TOKEN:
                if(buff[i] != 0x0)
                {
                    tokens[count] = &buff[i];
                    state = START_OF_TOKEN;
                    ++count;
                }
                break;
            case START_OF_TOKEN:
                if(buff[i] == 0x0)
                {
                    state = SEARCHING_FOR_TOKEN;
                }
                break;
        }
    }

    #ifdef DEBUG_CONSOLE
    gotoxy(0,2);
    cprintf("%p  ", &tokens[0]);
    for(i = 0; i < count; ++i)
        cprintf("%s,", tokens[i]);
    cputs("\n\r");
    cgetc();
    #endif

    return count;
}

void fix_chars(char* buff)
{
    byte i;
    for(i=0;i<MAX_LINE_LEN;++i)
    {
        if(buff[i] <= 63)
            buff[i] += 32;
        /*
        if(buff[i] == 0x0E)   // Change _ to .
            buff[i] = 0x2E;
        else if(buff[i] == 26) // :
            buff[i] = 58;
        else if(buff[i] > 15 && buff[i] < 26)
            buff[i] += 32;
            */
    }
        
}

void process_command(byte ntokens)
{
    if(!ntokens)
        return;

    if(strncmp(tokens[0], "help", 4) == 0)
    {
        byte old_console_lines = console_lines;
        const char help[] =
        "help - This screan\n\r"
        "quit - Exit this utility\n\r"
        "cls  - Clear the image display\n\r"
        "gfx  - [0,8,9] Set the graphics mode\n\r"
        "load - [filename] Load and display file\n\r"
        "save - [filename] Save memory to YAI\n\r"
        "\n\r"
        "Any key to continue...\n\r";

        console_lines = 8;
        enableConsole();
        reset_console();
        cputs(help);                              // Show the help text
        cgetc();                                  // Wait
        clrscr();
        console_lines = old_console_lines;
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
                case 16:
                    setGraphicsMode(GRAPHICS_0);
                    break;
                case 24:
                    setGraphicsMode(GRAPHICS_8);
                    break;
                case 25:
                    setGraphicsMode(GRAPHICS_9);
                    break;
                case 17:
                    switch(tokens[1][1])
                    {   case 16:
                            setGraphicsMode(GRAPHICS_10);
                            break;
                        case 17:
                            setGraphicsMode(GRAPHICS_11);
                            break;
                    }
                    break;
            }
        }
    }

    if(strncmp(tokens[0], "cls", 3) == 0)
    {
        clearFrameBuffer();
    }

    if(strncmp(tokens[0], "load", 4) == 0)
    {
        if(ntokens > 1)
        {
            fix_chars(tokens[1]);
            loadFile(tokens[1]);
        }
        else
        {
            gotoxy(0,0);
            clrscr();
            cprintf("ERROR: File not specified");
        }
    }

    if(strncmp(tokens[0], "save", 4) == 0)
    {
        if(ntokens > 1)
            saveFile(tokens[1]);

        else
        {
            gotoxy(0,0);
            cprintf("ERROR: File not specified");
        }
    }

    if(strncmp(tokens[0], "set", 3) == 0)
    {
        if(ntokens < 3)
        {
            gotoxy(0,0);
            cprintf("ERROR: Must specify a setting and value");
        }
        else
        {
            if(strncmp(tokens[1], "server", 3) == 0)
            {
                strncpy(server, tokens[2], 79);
            }
        }
    }

    if(strncmp(tokens[0], "stream", 3) == 0)
    {
        if(ntokens < 2)
        {
            gotoxy(0,0);
            cprintf("ERROR: Genre not specified");
        }
        else
        {
            loadImage(server, &tokens[1]);
        }
    }


}

void console_update(void)
{
    byte x = 0;
    if(!console_buff)
        console_buff = OS.savmsc;

    reset_console();

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
            return;
        }
   
        // Handle command
        switch(input)
        {
            case CH_ENTER:
            {
                // process the tokens
                #define WORKING_BUFF_SIZE 80
                byte buff[80];  // two lines of data
                byte ntokens = 0;

                memcpy(buff, console_buff, WORKING_BUFF_SIZE);

                #ifdef DEBUG_CONSOLE
                gotoxy(0,1);
                cprintf("%s\n\r", console_buff);
                cprintf("%s", buff);
                cgetc();
                #endif

                cursor(0);

                ntokens = get_tokens(buff, x + 1);

                #ifdef DEBUG_CONSOLE
                gotoxy(0,4);
                cprintf("%d  ", ntokens);
                #endif

                #ifdef DEBUG_CONSOLE
                // For debugging purposes, should display below the input
                // line which will not be shown when the system is active.
                {
                    byte i;

                    //memcpy(&line[40], line, 40);

                    cclearxy(0, 4, 40);
                    // gotoxy(0,4);
                    // for(i = 0; i < x; i++)
                    //     cprintf("%02x ", line[i]);

                    #define TOKEN_DBG_LINE 1
                    gotoxy(0,TOKEN_DBG_LINE);
                    cprintf("%d\n\r", ntokens);
                    for(i = 0; i < ntokens; i++)
                    {
                        cclearxy(0, i+TOKEN_DBG_LINE, 40);
                        gotoxy(0,i+TOKEN_DBG_LINE);
                        cprintf("%d: %s", i, tokens[i]);
                    }

                    cgetc();
                }
                #endif

                if(ntokens > 0)
                {
                    console_state = FALSE;
                    disableConsole();

                    process_command(ntokens);
                }
                else
                {
                    if(console_state)
                    {
                        console_state = FALSE;
                        disableConsole();
                    }
                    else
                    {
                        console_state = TRUE;
                        enableConsole();
                    }
                }

                reset_console();
            }
            break;

            case CH_DEL:
                if(x > 0)
                {
                    console_buff[x-1] = 0x0;
                    gotox(x-1);
                }
            break;

            default:
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
                if(!console_state)
                {
                    console_state = TRUE;
                    enableConsole();
                }

                cursor(1);
                cputc(input);
        }
    }
}

#endif