// Copyright (C) 2021 Brad Colbert
#include "fujinet-network.h"
#include "graphics.h"
#include "netimage.h"
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
extern byte CURRENT_MODE;

void stream_image(char* url, char* args[])
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
        network_close(url);
        return;
    }

    if(FN_ERR_OK != network_open(url, 12, 0))
    {
        show_console();
        cprintf("Failed to open %s\n\r", url);
        network_close(url);
        return;
    }

    // Send which graphics mode we are in
    memset(buff, 0, 256);
    sprintf(buff, "gfx %d ", CURRENT_MODE &= ~GRAPHICS_CONSOLE_EN);
    if(FN_ERR_OK != network_write(url, buff, 6))
    {
        show_console();
        cprintf("Unable to write graphics mode \"%s\"\n\r", buff);
        network_close(url);
        return;
    }

    // Build up the search string
    memset(buff, 0, 256);
    memcpy(buff, "search \"", 8);
    for(i = 0; i < 8; ++i)
    {
        if(0x0 == args[i])
            break;

        if(i > 0)
            strcat(buff, " ");
        strcat(buff, args[i]);
    }
    strcat(buff, "\"");

    i = strlen(buff);

    if(FN_ERR_OK != network_write(url, buff, i))
    {
        show_console();
        cprintf("Unable to write request\n\r");
        network_close(url);
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
            buffer_start = image.data;
            block_size = DISPLAYLIST_BLOCK_SIZE;
            lines_per_block = (ushort)(block_size/bytes_per_line);
            dl_block_size = lines_per_block * bytes_per_line;
            ttl_buff_size = lines * bytes_per_line;
            read_size = dl_block_size;

            // Read the header
            if(FN_ERR_OK != network_read(url, (unsigned char*)&image.header, sizeof(image.header)))
            {
                show_console();
                cprintf("Error reading\n\r");
                network_close(url);
                break;
            }

            setGraphicsMode(image.header.gfx);

            while(ttl_buff_size > 0)
            {
                if(read_size > ttl_buff_size)
                    read_size = ttl_buff_size;

                clrscr();
                if(FN_ERR_OK != network_read(url, buffer_start, read_size))
                {
                    show_console();
                    cprintf("Error reading\n\r");
                    network_close(url);
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

            if(FN_ERR_OK != network_write(url, "next", 4))
            {
                show_console();
                cprintf("Unable to write request\n\r");
                break;
            }
        }

        OS.atract = 0x00;   // disable attract mode
    }

    if(FN_ERR_OK != network_write(url, "quit", 4))
    {
        show_console();
        cprintf("Unable to write request\n\r");
    }

    network_close(url);

    OS.soundr = 3; // Restore SIO beeping sound
}
