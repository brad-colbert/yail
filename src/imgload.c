// Copyright (C) 2021 Brad Colbert

#include <atari.h>
#include <conio.h>
#include <peekpoke.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "nio.h"

//
#define TRANSLATION_NONE 0

//
struct image_header
{
    unsigned char v1;
    unsigned char v2;
    unsigned char v3;
    unsigned char gfx;
    unsigned char memtkn;
};

//
struct image_data
{
    //struct image_header header;
    unsigned char v1;
    unsigned char v2;
    unsigned char v3;
    unsigned char gfx;
    unsigned char memtkn;

    unsigned char data[8800];
};

//
char url[] = "N:TCP://192.168.1.205:9999/";                  // URL
bool running=true;              // Is program running?
char tmp[8];                    // temporary # to string
unsigned char err;              // error code of last operation.
unsigned char trip=0;           // if trip=1, fujinet is asking us for attention.
bool old_enabled=false;         // were interrupts enabled for old vector
void* old_vprced;               // old PROCEED vector, restored on exit.
//unsigned char rx_buf[8807];     // RX buffer.
struct image_data image;
unsigned char tx_buf[] = "search funny";       // TX buffer.
unsigned char quit_buf[] = "quit";       // TX buffer.
bool echo=false;                // local echo?
unsigned char txbuflen = 12;         // TX buffer length
unsigned char i;
char login[256];                // username for login
char password[256];             // password for login
unsigned short bw=0;            // # of bytes waiting.
unsigned short read_bytes = 0;  // state of read buffer
unsigned short ttl_bytes = sizeof(image); // state of read buffer

extern void ih();               // defined in intr.s

signed char enable_network(const char* url)
{
    err = nopen(url, TRANSLATION_NONE);

    if (err != SUCCESS)
    {
        cprintf("OPEN ERROR: %d", err);
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

    err = nclose(url);

    if (err != SUCCESS)
    {
        cprintf("CLOSE ERROR: %d", err);
        return -1;
    }

    return 0;
}

signed char check_network(const char* url)
{
    err = nstatus(url);

    if (err == 136)
    {
        cprintf("DISCONNECTED.\x9b");
        return -1;
    }
    else if (err != 1)
    {
        cprintf("STATUS ERROR: ");
        return -1;
    }

    return 0;
}

signed char write_network(const char* url, const char* buf, unsigned short len)
{
    err = nwrite(url, buf, len);

    if (err != 1)
    {
        cprintf("WRITE ERROR: ");
        return -1;
    }

    return 0;
}

signed char read_network(const char* url, unsigned char* buf, unsigned short len)
{
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
            cprintf("\rBw %d Ttl %d Read %d\n\r", bw, len - numread, numread);

            err = nread(url, buf + numread, bw);

            if (err != 1)
            {
                cprintf("READ ERROR: %d", err);
                running=false;
                continue;
            }

            // Print the buffer to screen.
            cprintf("\rRead %d bytes\n\r", bw);
            
            trip = 0;
            PIA.pactl |= 1; // Flag interrupt as serviced, ready for next one.
            numread += bw;  // Keep track of the number of bytes read
        } // if bw > 0

        if (numread == len)
        {
            cprintf("Data received\n\r");
            running=false;
        }

    } // while running

    return 0;
}

int main(int argc, char* argv[])
{
    OS.soundr=0; // Turn off SIO beeping sound
    cursor(1);   // Keep cursor on

    // Clear the edit buffer so as not to confuse our console code.
    clrscr();

    OS.lmargn=0; // Set left margin to 0
    OS.shflok=0; // turn off shift-lock.

    // Attempt open.
    cprintf("\x9bOpening:\x9b");
    cprintf(url);
    cprintf("\x9b");

    if (enable_network(url) < 0)
    {
        cprintf("Failed to open network\n\r");
        disable_network(url);
        return -1;
    }

    if(write_network(url, tx_buf, txbuflen) < 0)  // Send the request
    {
        cprintf("Unable to write request\n\r");
        disable_network(url);
        return -1;
    }

    if(read_network(url, (unsigned char*)&image, sizeof(image)) < 0)
    {
        cprintf("Error reading\n\r");
        disable_network(url);
        return -1;
    }

    cprintf("ver %d.%d.%d  gfx %d  mtkn %d\n", image.v1, image.v2, image.v3, image.gfx, image.memtkn);

    if(write_network(url, "quit", 4) < 0)  // Send the request
    {
        cprintf("Unable to write request\n\r");
        disable_network(url);
        return -1;
    }

    disable_network(url);

    OS.soundr=3; // Restore SIO beeping sound

    return 0;
}

#if 0

#include "console.h"
#include "graphics.h"
#include "files.h"
#include "utility.h"
#include "netimage.h"

#include <atari.h>
#include <conio.h>
#include <peekpoke.h>

#include <stdlib.h>
#include <string.h>

//
extern GfxDef gfxState;
extern char server[80];
const char* terms[] = {"funny", 0x0};

//
int main(int argc, char* argv[])
{
    byte i = 0;

    // Clear the edit buffer so as not to confuse our console code.
    clrscr();

    // Initialize everything
    memset(&gfxState, 0, sizeof(GfxDef));

    loadImage(server, terms);

    #if 0
    gfxState.dl.address = NULL;  // make sure it gets created
    gfxState.mode = 0xFF;        // make sure the mode get's set

    saveCurrentGraphicsState();
    setGraphicsMode(GRAPHICS_8);
    clearFrameBuffer();

    if(argc > 1)
        loadFile(argv[1]);

    else
        // Start user input.
        enableConsole();

    console_update();
    disableConsole();

    // Restore the graphics state back to the starting state.
    setGraphicsMode(GRAPHICS_0);
    restoreGraphicsState();
    clrscr();
    #endif

    return 0;
}

#endif