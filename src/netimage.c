// Copyright (C) 2021 Brad Colbert
#include "fujinet-network.h"
#include "graphics.h"
#include "netimage.h"
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

void stream_image(char* args[])
{
    const ushort bytes_per_line = 40;
    const ushort lines = 220;
    ushort buffer_start;
    ushort block_size;
    ushort lines_per_block;
    ushort dl_block_size;
    ushort ttl_buff_size;
    ushort read_size;
    ushort i;

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
        show_console();
        cputs("Failed to initialize network\n\r");
        network_close(settings.url);
        return;
    }

    if(FN_ERR_OK != network_open(settings.url, 12, 0))
    {
        show_console();
        cprintf("Failed to open %s\n\r", settings.url);
        network_close(settings.url);
        return;
    }

    // Send which graphics mode we are in
    memset(buff, 0, 256);
    sprintf((char*)buff, "gfx %d ", settings.gfx_mode &= ~GRAPHICS_CONSOLE_EN);
    if(FN_ERR_OK != network_write(settings.url, buff, 6))
    {
        show_console();
        cprintf("Unable to write graphics mode \"%s\"\n\r", buff);
        network_close(settings.url);
        return;
    }

    if(0 == strncmp(args[0], "http", 4))
    {
        // Build up the search string
        memset(buff, 0, 256);
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
    else
    {
        // Build up the search string
        memset(buff, 0, 256);
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

    i = strlen((char*)buff);

    if(FN_ERR_OK != network_write(settings.url, buff, i))
    {
        show_console();
        cprintf("Unable to write request\n\r");
        network_close(settings.url);
        return;
    }

    while(true)
    {
        if(kbhit())
        {
            cgetc();
            break;
        }
        else
        {
            buffer_start = (ushort)image.data;
            block_size = DISPLAYLIST_BLOCK_SIZE;
            lines_per_block = (ushort)(block_size/bytes_per_line);
            dl_block_size = lines_per_block * bytes_per_line;
            ttl_buff_size = lines * bytes_per_line;
            read_size = dl_block_size;

            // Read the header
            if(FN_ERR_OK != network_read(settings.url, (unsigned char*)&image.header, sizeof(image.header)))
            {
                show_console();
                cprintf("Error reading\n\r");
                network_close(settings.url);
                break;
            }

            setGraphicsMode(image.header.gfx);

            while(ttl_buff_size > 0)
            {
                if(read_size > ttl_buff_size)
                    read_size = ttl_buff_size;

                clrscr();
                if(FN_ERR_OK != network_read(settings.url, (uint8_t*)buffer_start, read_size))
                {
                    show_console();
                    cprintf("Error reading\n\r");
                    network_close(settings.url);
                    break;
                }

                buffer_start = buffer_start + block_size;
                ttl_buff_size = ttl_buff_size - read_size;
            }

            // Wait for keypress
            i = 0;
            while(i++ < 30000)   // roughly 5 seconds
                if(kbhit())
                    break;

            if(FN_ERR_OK != network_write(settings.url, (uint8_t*)"next", 4))
            {
                show_console();
                cprintf("Unable to write request\n\r");
                break;
            }
        }

        OS.atract = 0x00;   // disable attract mode
    }

    if(FN_ERR_OK != network_write(settings.url, (uint8_t*)"quit", 4))
    {
        show_console();
        cprintf("Unable to write request\n\r");
    }

    network_close(settings.url);

    OS.soundr = 3; // Restore SIO beeping sound
}
