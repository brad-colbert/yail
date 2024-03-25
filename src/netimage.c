// Copyright (C) 2021 Brad Colbert
#include "fujinet-network.h"
#include "graphics.h"
#include "netimage.h"
#include "utility.h"
#include "settings.h"
#include "types.h"

#include <atari.h>
#include <conio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

//
extern byte buff[];
extern ImageData image;
extern Settings settings;

void show_error_and_close_network(const char* message)
{
    show_error(message);
    network_close(settings.url);
}

char stream_image(char* args[], const byte video)
{
    ushort i = 0;
    char input;

    ushort buffer_start;
    ushort ttl_buff_size;
    ushort read_size;

    OS.soundr = 0; // Turn off SIO beeping sound

    #if DEBUG
    cprintf("reading from %s\n\r", url);
    for(i = 0; args[i] != 0x0; i++)
        cprintf("%s ", args[i]);
    cputs("\n\r");
    #endif

    hide_console();

    if(FN_ERR_OK != network_init())
    {
        show_error_and_close_network("Failed to initialize network\n\r");
        return 0x0;
    }

    if(FN_ERR_OK != network_open(settings.url, 12, 0))
    {
        show_error_and_close_network("Failed to open URL\n\r");
        return 0x0;
    }

    // Send which graphics mode we are in
    memset(buff, 0, 256);
    sprintf((char*)buff, "gfx %d ", settings.gfx_mode &= ~GRAPHICS_CONSOLE_EN);
    if(FN_ERR_OK != network_write(settings.url, buff, 6))
    {
        show_error_and_close_network("Unable to write graphics mode\n\r");
        return 0x0;
    }

    memset(buff, 0, 256);
    if(0 == strncmp(args[0], "http", 4))
    {
        // Build up the search string
        memcpy(buff, "showurl ", 8);
        for(i = 0; i < 8; ++i)
        {
            if(0x0 == args[i])
                break;

            if(i > 0)
                strcat(buff, " ");
            strcat(buff, args[i]);
        }
    }
    else if(args[0])
    {
        // Build up the search string
        memcpy(buff, "search \"", 8);
        for(i = 0; i < 8; ++i)
        {
            if(0x0 == args[i])
                break;

            if(i > 0)
                strcat((char*)buff, " ");
            strcat((char*)buff, args[i]);
        }
        strcat((char*)buff, "\"");
    }
    else if(video)
        memcpy(buff, "video", 8);

    i = strlen((char*)buff);

    if(FN_ERR_OK != network_write(settings.url, buff, i))
    {
        show_error_and_close_network("Unable to write request\n\r");
        return 0x0;
    }

    // We are starting streaming so remove the attract mode disable VBI because we will
    // disable attract mode ourselves.
    //remove_attract_disable_vbi();

    while(true)
    {
        // Read the header
        if(FN_ERR_OK != network_read(settings.url, (unsigned char*)&image.header, sizeof(image.header)))
        {
            show_error_and_close_network("Error reading\n\r");
            break;
        }

        // Read the image data
        #if 0

        #else
        if(video)
        {
            // Load data into the buffer that isn't being shown
            #define BUFFER_ONE_BLOCK_ONE_AND_TWO_SIZE 4080
            #define BUFFER_ONE_BLOCK_THREE_SIZE 640
            #define SWAP_DISPLAY_LISTS
            #ifdef SWAP_DISPLAY_LISTS
            if(settings.gfx_mode & GRAPHICS_BUFFER_TWO)
            {
                if(FN_ERR_OK != network_read(settings.url, (uint8_t*)image.data, BUFFER_ONE_BLOCK_ONE_AND_TWO_SIZE))
                {
                    show_error_and_close_network("Error reading\n\r");
                    break;
                }
                if(FN_ERR_OK != network_read(settings.url, (uint8_t*)image.data+0x1000, BUFFER_ONE_BLOCK_ONE_AND_TWO_SIZE))
                {
                    show_error_and_close_network("Error reading\n\r");
                    break;
                }
                if(FN_ERR_OK != network_read(settings.url, (uint8_t*)image.data+0x2000, BUFFER_ONE_BLOCK_THREE_SIZE))
                {
                    show_error_and_close_network("Error reading\n\r");
                    break;
                }
            }
            else
            #endif
            {
                #ifdef SWAP_DISPLAY_LISTS
                #define BUFFER_TWO_BLOCK_ONE_SIZE 3440
                #define BUFFER_TWO_BLOCK_TWO_SIZE 4080
                #define BUFFER_TWO_BLOCK_THREE_SIZE 1280
                if(FN_ERR_OK != network_read(settings.url, (uint8_t*)0x8280, BUFFER_TWO_BLOCK_ONE_SIZE))
                {
                    show_error_and_close_network("Error reading\n\r");
                    break;
                }
                if(FN_ERR_OK != network_read(settings.url, (uint8_t*)0x9000, BUFFER_TWO_BLOCK_TWO_SIZE))
                {
                    show_error_and_close_network("Error reading\n\r");
                    break;
                }
                if(FN_ERR_OK != network_read(settings.url, (uint8_t*)0xA000, BUFFER_TWO_BLOCK_THREE_SIZE))
                {
                    show_error_and_close_network("Error reading\n\r");
                    break;
                }
                #else
                if(FN_ERR_OK != network_read(settings.url, (uint8_t*)0x8280, 8800))
                {
                    show_error_and_close_network("Error reading\n\r");
                    break;
                }
                #endif
            }

            #ifdef SWAP_DISPLAY_LISTS
            setGraphicsMode((settings.gfx_mode & 0xEF) ^ GRAPHICS_BUFFER_TWO);

            // Delay for a bit to make sure the DLs have swapped.  Waiting for the VBI to finish
            wait_vbi();

            #else
            memcpy((void*)image.data, (void*)0x8280, BUFFER_ONE_BLOCK_ONE_AND_TWO_SIZE);
            memcpy((void*)(image.data+0x1000), (void*)(0x8280+BUFFER_ONE_BLOCK_ONE_AND_TWO_SIZE), BUFFER_ONE_BLOCK_ONE_AND_TWO_SIZE);
            memcpy((void*)(image.data+0x2000), (void*)(0x8280+BUFFER_ONE_BLOCK_ONE_AND_TWO_SIZE+BUFFER_ONE_BLOCK_ONE_AND_TWO_SIZE), BUFFER_ONE_BLOCK_THREE_SIZE);
            #endif
            #endif
        } // video
        else
        {
            buffer_start = (ushort)image.data;
            #define LINES_PER_BLOCK (DISPLAYLIST_BLOCK_SIZE/GFX_8_MEM_LINE)
            #define DL_BLOCK_SIZE (LINES_PER_BLOCK * GFX_8_MEM_LINE)
            ttl_buff_size = GFX_8_LINES * GFX_8_MEM_LINE;
            read_size = DL_BLOCK_SIZE;

            setGraphicsMode(image.header.gfx);

            // Delay for a bit to make sure the DLs have swapped.  Waiting for the VBI to finish
            wait_vbi();

            while(ttl_buff_size > 0)
            {
                if(read_size > ttl_buff_size)
                    read_size = ttl_buff_size;

                clrscr();
                if(FN_ERR_OK != network_read(settings.url, (uint8_t*)buffer_start, read_size))
                {
                    show_error_and_close_network("Error reading\n\r");
                    break;
                }

                buffer_start = buffer_start + DISPLAYLIST_BLOCK_SIZE;
                ttl_buff_size = ttl_buff_size - read_size;
            }            
        }

        // Wait for keypress
        i = 0;
        while(i++ < 30000)   // roughly 5 seconds
            if(kbhit())
            {
                input = cgetc();
                if(CH_ENTER == input)   // pause
                {
                    cgetc();            // any key to resume
                    input = 0x0;
                }
                else  // a key was pressed so let's assume it's a command and process it by quitting and returning the key
                {
                    show_console();
                    goto quit;
                }
            }

        if(FN_ERR_OK != network_write(settings.url, (uint8_t*)"next", 4))
        {
            show_error("Unable to write request\n\r");
            break;
        }

        OS.atract = 0x00;   // disable attract mode
    }

quit:
    if(FN_ERR_OK != network_write(settings.url, (uint8_t*)"quit", 4))
        show_error("Unable to write quit request\n\r");

    network_close(settings.url);

    OS.soundr = 3; // Restore SIO beeping sound

    // We are no longer streaming so disable attract mode with a VBI
    //add_attract_disable_vbi();

    return input;
}
