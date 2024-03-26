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

char check_keypress(ushort delay)
{
    // Wait for keypress
    char input;
    ushort i = 0;
    while(i++ < delay)   // roughly 5 seconds
        if(kbhit())
        {
            input = cgetc();
            if(CH_ENTER == input)   // pause
            {
                cgetc();            // any key to resume
                return(0x0);
            }
            else  // a key was pressed so let's assume it's a command and process it by quitting and returning the key
                show_console();
                return input;
        }

    return 0x0;
}

byte load_front_buffer()
{
    #define BUFFER_ONE_BLOCK_ONE_AND_TWO_SIZE 4080
    #define BUFFER_ONE_BLOCK_THREE_SIZE 640

    byte status = network_read(settings.url, (uint8_t*)image.data, BUFFER_ONE_BLOCK_ONE_AND_TWO_SIZE);
    if(FN_ERR_OK != status)
        goto quit;

    status = network_read(settings.url, (uint8_t*)image.data+0x1000, BUFFER_ONE_BLOCK_ONE_AND_TWO_SIZE);
    if(FN_ERR_OK != status)
        goto quit;

    status = network_read(settings.url, (uint8_t*)image.data+0x2000, BUFFER_ONE_BLOCK_THREE_SIZE);
    if(FN_ERR_OK != status)
        goto quit;

quit:
    return status;
}

byte load_back_buffer()
{
    #define BUFFER_TWO_BLOCK_ONE_SIZE 3440
    #define BUFFER_TWO_BLOCK_TWO_SIZE 4080
    #define BUFFER_TWO_BLOCK_THREE_SIZE 1280

    byte status = network_read(settings.url, (uint8_t*)0x8280, BUFFER_TWO_BLOCK_ONE_SIZE);
    if(FN_ERR_OK != status)
        goto quit;

    status = network_read(settings.url, (uint8_t*)0x9000, BUFFER_TWO_BLOCK_TWO_SIZE);
    if(FN_ERR_OK != status)
        goto quit;

    status = network_read(settings.url, (uint8_t*)0xA000, BUFFER_TWO_BLOCK_THREE_SIZE);
    if(FN_ERR_OK != status)
        goto quit;

quit:
    return status;
}

char stream_image(char* args[], const byte video)
{
    ushort i = 0;
    char input;

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
    remove_attract_disable_vbi();

    while(true)
    {
        // Read the header
        if(FN_ERR_OK != network_read(settings.url, (unsigned char*)&image.header, sizeof(image.header)))
        {
            show_error_and_close_network("Error reading\n\r");
            break;
        }

        // Read the image data
        if(video)
        {
            // Load data into the buffer that isn't being shown
            #define SWAP_DISPLAY_LISTS
            #ifdef SWAP_DISPLAY_LISTS
            if(settings.gfx_mode & GRAPHICS_BUFFER_TWO)
            {
                if(FN_ERR_OK != load_front_buffer())
                {
                    show_error_and_close_network("Error reading front buffer\n\r");
                    break;
                }
            }
            else
            #endif
            {
                #ifdef SWAP_DISPLAY_LISTS
                if(FN_ERR_OK != load_back_buffer())
                {
                    show_error_and_close_network("Error reading back buffer\n\r");
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
            #define BUFFER_ONE_BLOCK_ONE_AND_TWO_SIZE 4080
            #define BUFFER_ONE_BLOCK_THREE_SIZE 640
            memcpy((void*)image.data, (void*)0x8280, BUFFER_ONE_BLOCK_ONE_AND_TWO_SIZE);
            memcpy((void*)(image.data+0x1000), (void*)(0x8280+BUFFER_ONE_BLOCK_ONE_AND_TWO_SIZE), BUFFER_ONE_BLOCK_ONE_AND_TWO_SIZE);
            memcpy((void*)(image.data+0x2000), (void*)(0x8280+BUFFER_ONE_BLOCK_ONE_AND_TWO_SIZE+BUFFER_ONE_BLOCK_ONE_AND_TWO_SIZE), BUFFER_ONE_BLOCK_THREE_SIZE);
            #endif

            input = check_keypress(2);
            if(input)
                goto quit;

        } // video
        else
        {
            if(FN_ERR_OK != load_front_buffer())
            {
                show_error_and_close_network("Error reading front buffer\n\r");
                break;
            }

            input = check_keypress(30000);
            if(input)
                goto quit;
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
    add_attract_disable_vbi();

    return input;
}
