// Copyright (C) 2021 Brad Colbert

#ifndef USE_ORIGINAL

#define DEBUG
#include "nio.h"

#include <atari.h>
#ifdef DEBUG
#include <conio.h>
#endif

#include <stdbool.h>

//
#define TRANSLATION_NONE 0

unsigned char err;              // error code of last operation.
unsigned char trip=0;           // if trip=1, fujinet is asking us for attention.
bool old_enabled=false;         // were interrupts enabled for old vector
void* old_vprced;               // old PROCEED vector, restored on exit.

extern void ih();               // defined in intr.s

signed char enable_network(const char* url)
{
    err = nopen(url, TRANSLATION_NONE);

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

    err = nclose(url);

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
    err = nstatus(url);

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
    err = nwrite(url, buf, len);

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

            err = nread(url, buf + numread, bw);

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

void loadImage(char* url, char* args[])
{
}

#else

#include "graphics.h"
#include "utility.h"
#include "nio.h"
#include "types.h"

#include <atari.h>
#include <conio.h>

#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#define DEBUG_NETIMAGE

#define NO_STATE 0
#define VERSION_MAJ 10
#define VERSION_MIN 11
#define VERSION_BLD 12
#define GRAPHICS 13
#define SIZE 14
#define HEADER 15
#define DATA 16
#define DONE 17

extern GfxDef gfxState;
extern void ih();               // defined in intr.s

void* old_vprced;               // old PROCEED vector, restored on exit.
bool old_enabled=false;         // were interrupts enabled for old vector
bool running = true;
unsigned char trip=0;           // if trip=1, fujinet is asking us for attention.
unsigned short bw=0;            // # of bytes waiting.
unsigned ttl_bytes = 0;
unsigned seg_used = 0;
byte seg_count = 0;
MemSeg* seg = NULL;
byte i = 0;
byte err;
byte state = VERSION_MAJ;
byte maj = 0, min = 0, bld = 0;
byte gfx = 0;
size_t image_bytes = 0;
byte header = 0;
bool img_load_done;

//char test[] = "search cars";
char search_command[80];
char stop_cmd[] = "stop";
char next_cmd[] = "next";

void loadImage(char* url, char* args[])
{
    running = true;
    trip = 0;           // if trip=1, fujinet is asking us for attention.
    bw = 0;            // # of bytes waiting.
    ttl_bytes = 0;
    seg_used = 0;
    seg_count = 0;
    seg = NULL;
    i = 0;
    state = VERSION_MAJ;
    gfx = 0;
    image_bytes = 0;
    header = 0;
    img_load_done = false;

    #ifdef DEBUG_NETIMAGE
//    enableConsole();
    cprintf("Opening %s\n\r", url);
    #endif

    err = nopen(url, 0);

    if (err != SUCCESS)
    {
        cprintf("OPEN ERROR: ");
        //cprint_error(err);
        return;
    }

    #ifdef DEBUG_NETIMAGE
    cprintf("Opened %s\n\r", url);
    #endif

    // Open successful, set up interrupt
    old_vprced  = OS.vprced;     // save the old interrupt vector 
    old_enabled = PIA.pactl & 1; // keep track of old interrupt state
    PIA.pactl  &= (~1);          // Turn off interrupts before changing vector
    OS.vprced   = ih;            // Set PROCEED interrupt vector to our interrupt handler.
    PIA.pactl  |= 1;             // Indicate to PIA we are ready for PROCEED interrupt.

    // Concatinate the args into the search command sent to the server.
    memset(search_command, 0, 80);
    strncat(search_command, "search ", 80);
    while(args[i] != 0x0)
    {
        strcat(search_command, args[i++]);
        strcat(search_command, " ");
    }
    search_command[strlen(search_command)-1] = 0x0;
    #ifdef DEBUG_NETIMAGE
    cprintf("Search command %s\n\r", search_command);
    cgetc();
    #endif

    // Send the search command
    err = nwrite(url, search_command, strlen(search_command)-1);

    if (err != 1)
    {
        cprintf("WRITE ERROR: ");
        //print_error(err);
        running=false;
    }

    while (running == true)
    {
        if(kbhit())
        {
            #ifdef DEBUG_NETIMAGE
            clrscr();
            cputs("Key hit, trying to quit\n\r");
            //cgetc();
            #endif

            // Keypress stop this
            running = false;
            continue;
        }

        if (trip == 0) // is nothing waiting for us? puts us in jeopardy of a dead lock spin...
        {
            #ifdef DEBUG_NETIMAGE
            cputs("No data ");
            #endif
            continue;
        }

        #ifdef DEBUG_NETIMAGE
        clrscr();
        cputs("Data is waiting\n\r");
        cgetc();
        #endif

        // Something waiting for us, get status and bytes waiting.
        err = nstatus(url);

        if (err==136)
        {
            cprintf("DISCONNECTED.\x9b");
            running = false;
            continue;
        }
        else if (err!=1)
        {
            cprintf("STATUS ERROR: ");
            //print_error(err);
            running = false;
            continue;
        }

        // Get # of bytes waiting, no more than size of rx_buf
        bw = OS.dvstat[1] * 256 + OS.dvstat[0];

        #ifdef DEBUG_NETIMAGE
        //clrscr();
        cprintf("%d bytes waiting\n\r", bw);
        cgetc();
        #endif

        switch(state)
        {
            case VERSION_MAJ: // read first part of version
                if (bw>0)
                    err = nread(url, &maj, 1);

                #ifdef DEBUG_NETIMAGE
                //clrscr();
                cprintf("Read V Maj %d\n\r", maj);
                cgetc();
                #endif

                state = VERSION_MIN;
                break;

            case VERSION_MIN: // read first part of version
                if (bw>0)
                    err = nread(url, &min, 1);

                #ifdef DEBUG_NETIMAGE
                //clrscr();
                cprintf("Read V Min %d\n\r", min);
                cgetc();
                #endif

                state = VERSION_BLD;
                break;

            case VERSION_BLD: // read first part of version
                if (bw>0)
                    err = nread(url, &bld, 1);

                #ifdef DEBUG_NETIMAGE
                //clrscr();
                cprintf("Read V Bld %d\n\r", bld);
                cgetc();
                #endif

                state = GRAPHICS;
                break;

            case GRAPHICS: // read first part of version
                if (bw>0)
                    err = nread(url, &gfx, 1);

                #ifdef DEBUG_NETIMAGE
                //clrscr();
                cprintf("Read V Gfx %d\n\r", gfx);
                cgetc();
//                enableConsole();
                #else
                setGraphicsMode(gfx);
                disableConsole();
                #endif

                seg = &gfxState.buffer.segs[seg_count++];

                #if 0 //def DEBUG_NETIMAGE
                while(seg->size > 0)
                {
                    size_t len = (seg->size / seg->block_size) * seg->block_size;
                    cprintf("%d %p %d  ", seg_count, seg->addr, len);
                    seg = &gfxState.buffer.segs[++seg_count];
                }
                seg_count = 0;
                seg = &gfxState.buffer.segs[seg_count];
                ++seg_count;
                //cgetc();
                #endif

                state = HEADER;
                break;

            case HEADER: // read first part of version
                if (bw>0)
                    err = nread(url, &header, 1);

                #ifdef DEBUG_NETIMAGE
                //clrscr();
                cprintf("Read hdr %d\n\r", header);
                cgetc();
                #endif

                state = SIZE;
                break;

            case SIZE: // read first part of version
                if (bw > 0)
                    err = nread(url, (unsigned char*)&image_bytes, 2);

                #ifdef DEBUG_NETIMAGE
                //clrscr();
                cprintf("Read size %d\n\r", image_bytes);
                cgetc();
                #endif

                state = DATA;
                break;

            case DATA: // read first part of version
                if(header == 0x03)
                {
                    if((seg->size > 0) && (ttl_bytes < image_bytes))
                    {
                        if (bw > 0)
                        {
                            size_t seg_size = (seg->size / seg->block_size) * seg->block_size;
                            size_t seg_bytes_avail = seg_size - seg_used;
                            size_t bytes_to_copy = seg_bytes_avail;
                            void* dest_addr = seg->addr;

                            OS.soundr = 0; // Turn off SIO beeping sound

                            if(bw < seg_bytes_avail)
                                bytes_to_copy = bw;

                            dest_addr = (void*)((unsigned)seg->addr + seg_used);

                            if(bytes_to_copy > 0)
                            {
                                #ifdef DEBUG_NETIMAGE
                                cprintf("Reading %d bytes ", bytes_to_copy);
                                #endif
                                err = nread(url, dest_addr, bytes_to_copy);

                                seg_used += bytes_to_copy;
                                ttl_bytes += bytes_to_copy;
                                #ifdef DEBUG_NETIMAGE
                                cprintf("(%d, %d, %d)\n\r", seg_bytes_avail, ttl_bytes, image_bytes);
                                #endif
                            }
                            else
                            {
                                #ifdef DEBUG_NETIMAGE
                                cputs("0 bytes available\n\r");
                                #endif
                            }
                            

                            if(seg_bytes_avail == 0)
                            {
                                seg = &gfxState.buffer.segs[seg_count++];                                
                                seg_used = 0;
                            }

                            if(ttl_bytes >= image_bytes)
                            {
                                img_load_done = true;
                            }
                            // {
                            //     state = DONE;
                            //     running = false;
                            // }
                            OS.soundr = 3; // Turn SIO beep back
                        }
                    }
                    else
                    {
                        #ifdef DEBUG_NETIMAGE
                        //clrscr();
                        cputs("No more segs... done\n\r");
                        cgetc();
                        #endif

                        state = DONE;
                        running = false;
                        err = nwrite(url, stop_cmd, 4);
                        continue;
                    }
                }
                break;

            default:
                #ifdef DEBUG_NETIMAGE
                cputs("Done\n\r");
                #endif
                running = false;

        } // switch state

        if (err!=1)
        {
          cprintf("READ ERROR: ");
          //print_error(err);
          running = false;
          //continue;
        }

        trip = 0;
        PIA.pactl |= 1; // Flag interrupt as serviced, ready for next one.

        if(img_load_done)
        {
            ttl_bytes = 0;
            seg_used = 0;
            seg_count = 0;
            seg = NULL;
            i = 0;
            state = VERSION_MAJ;
            gfx = 0;
            image_bytes = 0;
            header = 0;
            img_load_done = false;
            err = nwrite(url, next_cmd, 4);  // get the next image
            sleep(3);
        }
    } // while

    #ifdef DEBUG_NETIMAGE
    cputs("Loop complete\n\r");
    //cgetc();
    #endif

    // Sending stop
    err = nwrite(url, stop_cmd, 4);

    // Restore old PROCEED interrupt.
    PIA.pactl &= ~1; // disable interrupts
    OS.vprced=old_vprced; 
    PIA.pactl |= old_enabled; 
}

#endif