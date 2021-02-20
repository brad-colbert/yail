// Copyright (C) 2021 Brad Colbert

#if 1

#include "nio.h"
#include "screen_buffers.h"
#include "types.h"

#include <atari.h>
#include <conio.h>

#include <string.h>
#include <stdbool.h>
#include <unistd.h>

//#define DEBUG_NETIMAGE

#define NO_STATE 0
#define VERSION_MAJ 10
#define VERSION_MIN 11
#define VERSION_BLD 12
#define GRAPHICS 13
#define SIZE 14
#define HEADER 15
#define DATA 16
#define DONE 17

extern void ih();               // defined in intr.s

void* old_vprced;               // old PROCEED vector, restored on exit.
bool old_enabled=false;         // were interrupts enabled for old vector
bool running = true;
unsigned char trip=0;           // if trip=1, fujinet is asking us for attention.
unsigned short bw=0;            // # of bytes waiting.
// unsigned ttl_bytes = 0;
// unsigned seg_used = 0;
size_t ttl_seg_bytes_read = 0;
byte seg_count = 0;
byte i = 0;
byte err;
byte state = VERSION_MAJ;
byte maj = 0, min = 0, bld = 0;
byte gfx = 0;
size_t image_bytes = 0;
byte header = 0;
bool img_load_done;

//char search_command[80];
char search_command[] = "search piper comanche p24";
char stop_cmd[] = "stop";
char next_cmd[] = "next";

void loadImage(char* url, char* args[])
{
    running = true;
    trip = 0;           // if trip=1, fujinet is asking us for attention.
    bw = 0;            // # of bytes waiting.
    ttl_seg_bytes_read = 0;
    // ttl_bytes = 0;
    // seg_used = 0;
    seg_count = 0;
    i = 0;
    state = VERSION_MAJ;
    gfx = 0;
    image_bytes = 0;
    header = 0;
    img_load_done = false;

    #ifdef DEBUG_NETIMAGE
    clrscr();
    cprintf("Opening %s\n\r", url);
    #endif

    err = nopen(url, 0);

    if (err != SUCCESS)
    {
        #ifdef DEBUG_NETIMAGE
        cprintf("OPEN ERROR: ");
        #endif
        return;
    }

    #ifdef DEBUG_NETIMAGE
    cprintf("Opened %s\n\r", url);
    #endif

    // Open successful, set up interrupt
    old_vprced  = OS.vprced;     // save the old interrupt vector 
    old_enabled = PIA.pactl & 1; // keep track of old interrupt state
    PIA.pactl  &= (~1);          // Turn off interrupts before changing vector
    OS.vprced   = (void*)ih;     // Set PROCEED interrupt vector to our interrupt handler.
    PIA.pactl  |= 1;             // Indicate to PIA we are ready for PROCEED interrupt.

    /* Generalized but need something less for testing
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
    */

    err = nwrite(url, search_command, strlen(search_command)-1); // Send character.

    if (err != 1)
    {
        #ifdef DEBUG_NETIMAGE
        cprintf("WRITE ERROR: ");
        #endif
        running=false;
    }

    while (true == running)
    {
        if(kbhit())
        {
            #ifdef DEBUG_NETIMAGE
            // clrscr();
            cputs("Key hit, trying to quit\n\r");
            cgetc();
            #endif

            // Keypress stop this
            running = false;
            continue;
        }

        if (trip == 0) // is nothing waiting for us? puts us in jeopardy of a dead lock spin...
        {
            #ifdef DEBUG_NETIMAGE
            // cputs("No data ");
            #endif
            continue;
        }

        #ifdef DEBUG_NETIMAGE
        // clrscr();
        cputs("Data is waiting\n\r");
        //cgetc();
        #endif

        // Something waiting for us, get status and bytes waiting.
        err = nstatus(url);

        if (err==136)
        {
            #ifdef DEBUG_NETIMAGE
            cprintf("DISCONNECTED.\x9b");
            #endif
            running = false;
            continue;
        }
        else if (err!=1)
        {
            #ifdef DEBUG_NETIMAGE
            cprintf("STATUS ERROR: ");
            #endif
            running = false;
            continue;
        }

        // Get # of bytes waiting, no more than size of rx_buf
        bw = OS.dvstat[1] * 256 + OS.dvstat[0];

        #ifdef DEBUG_NETIMAGE
        // clrscr();
        cprintf("%d bytes waiting\n\r", bw);
        cprintf("STATE %d                     \n\r", state);
        #endif

        switch(state)
        {
            case VERSION_MAJ: // read first part of version
                if (bw > 0)
                    err = nread(url, &maj, 1);

                #ifdef DEBUG_NETIMAGE
                // clrscr();
                cprintf("Read V Maj %d\n\r", maj);
                //cgetc();
                #endif

                state = VERSION_MIN;
                break;

            case VERSION_MIN: // read first part of version
                if (bw>0)
                    err = nread(url, &min, 1);

                #ifdef DEBUG_NETIMAGE
                // clrscr();
                cprintf("Read V Min %d\n\r", min);
                //cgetc();
                #endif

                state = VERSION_BLD;
                break;

            case VERSION_BLD: // read first part of version
                if (bw>0)
                    err = nread(url, &bld, 1);

                #ifdef DEBUG_NETIMAGE
                // clrscr();
                cprintf("Read V Bld %d\n\r", bld);
                //cgetc();
                #endif

                state = GRAPHICS;
                break;

            case GRAPHICS: // read graphics mode
                if (bw>0)
                    err = nread(url, &gfx, 1);

                #ifdef DEBUG_NETIMAGE
                // clrscr();
                cprintf("Read V Gfx %d\n\r", gfx);
                //cgetc();
                #else
                // setGraphicsMode(gfx);
                // disableConsole();
                #endif

                // seg = &gfxState.buffer.segs[seg_count++];

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

            case HEADER: // read image header
                if (bw>0)
                    err = nread(url, &header, 1);

                #ifdef DEBUG_NETIMAGE
                // clrscr();
                cprintf("Read hdr %d\n\r", header);
                //cgetc();
                #endif

                state = SIZE;
                break;

            case SIZE: // read size of image
                if (bw > 0)
                    err = nread(url, (byte*)&image_bytes, 2);

                #ifdef DEBUG_NETIMAGE
                // clrscr();
                cprintf("Read size %d\n\r", image_bytes);
                //cgetc();
                #endif

                state = DATA;
                break;

            case DATA: // read image data
                #ifdef DEBUG_NETIMAGE
                cputs("                                       \r");
                #endif
                if(header == 0x03)
                {
                    #ifdef DEBUG_NETIMAGE
                    cputs("DATA\n\r");
                    #endif
                    if(seg_count < 3)  // still buffer segments to fill
                    {
                        if(bw > 0)  // bytes are waiting
                        {
                            size_t seg_bytes_avail = green_buff_sizes[seg_count] - ttl_seg_bytes_read;

                            OS.soundr = 0; // turn off SIO beeping sound

                            if(bw > seg_bytes_avail)
                                bw = seg_bytes_avail;

                            err = nread(url, ((byte*)green_buff_segs[seg_count]) + ttl_seg_bytes_read, bw);

                            if(1 == err) // good read
                            {
                                ttl_seg_bytes_read += bw;

                                #ifdef DEBUG_NETIMAGE
                                cprintf("S(%d) read = %d, ttl = %d\n\r", seg_count, bw, ttl_seg_bytes_read);
                                #endif

                                if(ttl_seg_bytes_read >= green_buff_sizes[seg_count])
                                {
                                    ttl_seg_bytes_read = 0;
                                    ++seg_count;
                                    #ifdef DEBUG_NETIMAGE
                                    cprintf("SEGCOUNT UPDATED %d                   \n\r", seg_count);
                                    #endif
                                }

                                if(seg_count > 2)
                                {
                                    #ifdef DEBUG_NETIMAGE
                                    cputs("No more segs... image load done      \n\r");
                                    #endif

                                    img_load_done = true;
                                    break;
                                }
                            }
                            else // bad read
                            {
                                #ifdef DEBUG_NETIMAGE
                                cprintf("BAD READ DATA %d                   \n\r", err);
                                #endif
                            }

                            OS.soundr = 3; // Turn SIO beep back
                        }
                    }
                    else
                    {
                        #ifdef DEBUG_NETIMAGE
                        // clrscr();
                        cputs("No more segs... done\n\r");
                        //cgetc();
                        #endif

                        img_load_done = true;
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

        if (err != 1)
        {
            #ifdef DEBUG_NETIMAGE
            cprintf("READ ERROR: ");
            #endif
            running = false;
        }

        trip = 0;
        PIA.pactl |= 1; // Flag interrupt as serviced, ready for next one.

        if(img_load_done)
        {
            #ifdef DEBUG_NETIMAGE
            cputs("LOADING NEXT                     \n\r");
            cgetc();
            #endif
            ttl_seg_bytes_read = 0;
            // ttl_bytes = 0;
            // seg_used = 0;
            seg_count = 0;
            // seg = NULL;
            i = 0;
            state = VERSION_MAJ;
            gfx = 0;
            image_bytes = 0;
            header = 0;
            img_load_done = false;
            err = nwrite(url, next_cmd, 4);  // get the next image
            // sleep(3);
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
#else

unsigned char trip=0;           // if trip=1, fujinet is asking us for attention.

#endif