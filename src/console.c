// Copyright (C) 2021 Brad Colbert
#include "netimage.h"
#include "readNetPBM.h"
#include "graphics.h"
#include "console.h"
#include "files.h"
#include "consts.h"
#include "settings.h"
#include "types.h"
#include "fujinet-io.h"
#include "utility.h"

#include <atari.h>
#include <conio.h>
#include <peekpoke.h>

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Externs
extern char version[];
extern bool done;
extern byte IMAGE_FILE_TYPE;
extern void* ORG_SDLIST;
extern void graphics_8_console_dl[];
extern void graphics_9_console_dl[];
extern Settings settings;

// Globals
bool console_state = false;

#ifdef CONSOLE_USE_LOCAL_BUFFER
char CONSOLE_BUFF[GFX_0_MEM_LINE * CONSOLE_LINES];
#else
#define CONSOLE_BUFF ((byte*)((ushort*)ORG_SDLIST)[2])
#endif
char* tokens[8]; // = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };  // Maximum of 8 tokens

void reset_console(void)
{
    memset(CONSOLE_BUFF, 0, GFX_0_MEM_LINE * CONSOLE_LINES);//console_lines); // wipe all of the console mem
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
                    tokens[count] = (char*)&buff[i];
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

    // Fix last token.  For some strange reason, the last token is ending with 0x80 (ENTER key?)
    i = strlen(tokens[count-1]);
    tokens[count-1][i-1] = 0x0;

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

void process_command(byte ntokens)
{
    if(!ntokens)
        return;

    if(strncmp(tokens[0], "help", 4) == 0)
    {
        const char help[] =
        "quit - Exit this utility\n\r"
        "cls  - Clear the image display\n\r"
        "gfx  - [0,8,9] Set the graphics mode\n\r"
        "set  - Saved settings\n\r"
        "       server [url] (N:TCP://blah.duh/)\n\r"
        #ifdef YAIL_BUILD_FILE_LOADER
        "load - [filename] Load and display file\n\r"
        "save - [filename] Save memory to YAI\n\r";
        #else
        "stream - [arg0...argN] Stream images\n\r";
        #endif
        byte SAVED_MODE = settings.gfx_mode;

        setGraphicsMode(GRAPHICS_0);
        cputs(help);                              // Show the help text
        cgetc();                                  // Wait
        setGraphicsMode(SAVED_MODE);              // Restore the graphics mode
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
                    setGraphicsMode(GRAPHICS_0);
                    break;
                case '8':
                    setGraphicsMode(GRAPHICS_8);
                    break;
                case '9':
                    setGraphicsMode(GRAPHICS_9);
                    break;
                case '1':
                    switch(tokens[1][1])
                    {   case '0':
                            setGraphicsMode(GRAPHICS_10);
                            break;
                        case '1':
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
        #ifdef YAIL_BUILD_FILE_LOADER
        if(ntokens > 1)
        {
            //internal_to_atascii(tokens[1], 40);
            load_image_file(tokens[1]);
        }
        else
        {
            gotoxy(0,0);
            clrscr();
            cputs("ERROR: File not specified");
            cgetc();
        }
        #else
            gotoxy(0,0);
            clrscr();
            cputs("ERROR: File loading not supported");
            cgetc();
        #endif
    }

    if(strncmp(tokens[0], "save", 4) == 0)
    {
        #ifdef YAIL_BUILD_FILE_LOADER
        if(ntokens > 1)
            saveFile(tokens[1]);

        else
        {
            gotoxy(0,0);
            cprintf("ERROR: File not specified");
            cgetc();
        }
        #else
            gotoxy(0,0);
            clrscr();
            cputs("ERROR: File saving not supported");
            cgetc();
        #endif
    }

    if(strncmp(tokens[0], "set", 3) == 0)
    {
        if(ntokens < 3)
        {
            gotoxy(0,0);
            cputs("ERROR: Must specify a setting and value");
            cgetc();
        }
        else
        {
            if(strncmp(tokens[1], "server", 3) == 0)
            {
                //strncpy(server, tokens[2], 79);
                strncpy(settings.url, tokens[2], SERVER_URL_SIZE);
                put_settings(SETTINGS_URL);  // save the URL on the FN
            }
        }
    }

    if(strncmp(tokens[0], "stream", 3) == 0)
    {
        if(ntokens < 2)
        {
            gotoxy(0,0);
            cputs("ERROR: Search tokens not specified");
            cgetc();
        }
        else
        {
            #ifdef DEBUG_CONSOLE
            byte i;
            for(i = 0; tokens[i] != 0x0; i++)
                cprintf("*%s ", tokens[i]);

            cputs("\n\r");
            #endif

            stream_image(&tokens[1]);
        }
    }
}

void start_console(char first_char)
{
    byte* gfx8_console_dl = (byte*)graphics_8_console_dl;
    byte* gfx9_console_dl = (byte*)graphics_9_console_dl;
    byte* console_buff = (byte*)CONSOLE_BUFF;
    byte x = 0;

    // Fix addresses for graphics display lists for the console buffer
    POKEW(gfx8_console_dl + 2, (ushort)version);
    POKEW(gfx8_console_dl + 185, (ushort)console_buff);
    POKEW(gfx9_console_dl + 2, (ushort)version);
    POKEW(gfx9_console_dl + 185, (ushort)console_buff);

    reset_console();

    cursor(1);

    while(true)
    {
        byte input = first_char?first_char:cgetc();
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
                byte buff[WORKING_BUFF_SIZE];  // two lines of data
                byte ntokens = 0;

                memcpy(buff, CONSOLE_BUFF, WORKING_BUFF_SIZE);
                internal_to_atascii((char*)buff, WORKING_BUFF_SIZE);

                #ifdef DEBUG_CONSOLE
                gotoxy(0,1);
                cprintf("%s\n\r", CONSOLE_BUFF);
                cprintf("%s", buff);
                cgetc();
                #endif

                ntokens = get_tokens(buff, x);// + 1);

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
                    byte i;
                    reset_console();
                    process_command(ntokens);

                    // Clear the tokens for the next command
                    for(i = 0; i < 7; ++i)
                        tokens[i] = 0x0;
                }

                reset_console();
                hide_console();
                cursor(0);
                return;
            }
            break;

            case CH_DEL:
                if(x > 0)
                {
                    CONSOLE_BUFF[x-1] = 0x0;
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
                    console_state = true;
                    show_console();
                }
                cputc(input);
        }

        first_char = 0x00;
    }
}
