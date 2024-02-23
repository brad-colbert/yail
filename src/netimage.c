// Copyright (C) 2021 Brad Colbert
#include "nio.h"
#include "graphics.h"
#include "netimage.h"
#include "types.h"

#include <atari.h>
#include <conio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

//
#define TRANSLATION_NONE 0

//
extern byte framebuffer[FRAMEBUFFER_SIZE];
extern byte buff[];

//
extern ImageData image;

//
extern void ih();               // defined in intr.s
extern byte framebuffer[];      // defined in graphics.c

//
unsigned char err;              // error code of last operation.
unsigned char trip=0;           // if trip=1, fujinet is asking us for attention.
bool old_enabled=false;         // were interrupts enabled for old vector
void* old_vprced;               // old PROCEED vector, restored on exit.

signed char enable_network(const char* url)
{
    err = nopen((char*)url, TRANSLATION_NONE);

    if (err != SUCCESS)
    {
        #ifdef DEBUG
        cprintf("OPEN ERROR: %d", err);
        #endif
        return -1;
    }

    // Open successful, set up interrupt
    old_vprced  = OS.vprced;     // save the old interrupt vector 
    old_enabled = PIA.pactl & 1; // keep track of old interrupt state
    PIA.pactl  &= (~1);          // Turn off interrupts before changing vector
    OS.vprced   = ih;            // Set PROCEED interrupt vector to our interrupt handler.
    PIA.pactl  |= 1;             // Indicate to PIA we are ready for PROCEED interrupt.

    return 0;
}

signed char disable_network(const char* url)
{
    // Restore old PROCEED interrupt.
    PIA.pactl &= ~1; // disable interrupts
    OS.vprced=old_vprced; 
    PIA.pactl |= old_enabled;

    err = nclose((char*)url);

    if (err != SUCCESS)
    {
        #ifdef DEBUG
        cprintf("CLOSE ERROR: %d", err);
        #endif
        return -1;
    }

    return 0;
}

signed char check_network(const char* url)
{
    err = nstatus((char*)url);

    if (err == 136)
    {
        #ifdef DEBUG
        cprintf("DISCONNECTED.\x9b");
        #endif
        return -1;
    }
    else if (err != 1)
    {
        #ifdef DEBUG
        cprintf("STATUS ERROR: ");
        #endif
        return -1;
    }

    return 0;
}

signed char write_network(const char* url, const char* buf, unsigned short len)
{
    err = nwrite((char*)url, (byte*)buf, len);

    if (err != 1)
    {
        #ifdef DEBUG
        cprintf("WRITE ERROR: ");
        #endif
        return -1;
    }

    return 0;
}

signed char read_network(const char* url, unsigned char* buf, unsigned short len)
{
    bool running=true;              // Keep reading until we have read all data
    unsigned short bw=0;            // # of bytes waiting.
    unsigned short numread = 0;

    while(running == true)
    {
        if (trip == 0) // is nothing waiting for us?
            continue;

        if(check_network(url) < 0)
        {
            running = false;
            continue;
        }

        // Get # of bytes waiting, no more than size of rx_buf
        bw = OS.dvstat[1] * 256 + OS.dvstat[0];

        if (bw > (len - numread))
            bw = (len - numread);

        if (bw > 0)
        {
            #ifdef DEBUG
            cprintf("\rBw %d Ttl %d Read %d\n\r", bw, len - numread, numread);
            #endif

            err = nread((char*)url, (byte*)(buf + numread), bw);

            if (err != 1)
            {
                #ifdef DEBUG
                cprintf("READ ERROR: %d", err);
                #endif
                running=false;
                continue;
            }

            // Print the buffer to screen.
            #ifdef DEBUG
            cprintf("\rRead %d bytes\n\r", bw);
            #endif

            trip = 0;
            PIA.pactl |= 1; // Flag interrupt as serviced, ready for next one.
            numread += bw;  // Keep track of the number of bytes read
        } // if bw > 0

        if (numread == len)
        {
            #ifdef DEBUG
            cprintf("Data received\n\r");
            #endif
            running=false;
        }

    } // while running

    return 0;
}

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

    memset(buff, 0, 256);
    memcpy(buff, "search \"", 8);
    for(i = 0; i < 8; ++i)
    {
        if(args[i] == 0x0)
            break;

        if(i > 0)
            strcat(buff, " ");
        strcat(buff, args[i]);
    }
    strcat(buff, "\"");

    i = strlen(buff);

    #if DEBUG
    cprintf("Sending %s\n\r", buff);
    #endif

    hide_console();

    if (enable_network(url) < 0)
    {
        show_console();
        cprintf("Failed to open network\n\r");
        disable_network(url);
        return;
    }

    if(write_network(url, buff, i) < 0)  // Send the request
    {
        show_console();
        cprintf("Unable to write request\n\r");
        disable_network(url);
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
            buffer_start = framebuffer;
            block_size = DISPLAYLIST_BLOCK_SIZE;
            lines_per_block = (ushort)(block_size/bytes_per_line);
            dl_block_size = lines_per_block * bytes_per_line;
            ttl_buff_size = lines * bytes_per_line;
            read_size = dl_block_size;

            // Read the header
            if(read_network(url, (unsigned char*)&image.header, sizeof(image.header)) < 0)
            {
                show_console();
                cprintf("Error reading\n\r");
                disable_network(url);
                break;
            }

            setGraphicsMode(image.header.gfx);

            while(ttl_buff_size > 0)
            {
                if(read_size > ttl_buff_size)
                    read_size = ttl_buff_size;

                clrscr();
                //cprintf("Start %04X Read %04X\n\r", buffer_start, read_size);
                if(read_network(url, buffer_start, read_size) < 0)
                {
                    show_console();
                    cprintf("Error reading\n\r");
                    disable_network(url);
                    break;
                }

                buffer_start = buffer_start + block_size;
                ttl_buff_size = ttl_buff_size - read_size;
            }

            // Wait for keypress
            i = 0;
            while(i++ < 30000)
                if(kbhit())
                    break;

            if(write_network(url, "next", 4) < 0)  // Send the request
            {
                show_console();
                cprintf("Unable to write request\n\r");
                break;
            }
        }
    }

    if(write_network(url, "quit", 4) < 0)  // Send the request
    {
        show_console();
        cprintf("Unable to write request\n\r");
    }

    disable_network(url);

    OS.soundr = 3; // Restore SIO beeping sound
}
