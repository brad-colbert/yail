// Copyright (C) 2021 Brad Colbert

#include "displaylist.h"
#include "console.h"

#include <atari.h>
#include <conio.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//#define DEBUG_EXPAND_DL

// Expands a display list memory references over 4k memory boundaries
// Right now all lines are 40bytes
#define BYTES_PER_LINE 40
#define MAX_MEM_BLOCK 4096
#define MAX_LINES_PER_BLOCK 102
#define MEM_PER_BLOCK 4080
DLDefParray expandDisplayList(DLDef* dl)
{
    DLDefParray new_dl_defs;
    byte i = 0, count = 1;
    byte* memptr = (byte*)dl->address;
    unsigned memsize = dl->lines*BYTES_PER_LINE;
    unsigned memptr_4k = (unsigned)memptr + 4096;
    unsigned rem = memptr_4k % 4096;
    unsigned next_4k = memptr_4k - rem;
    unsigned line_count = 0;

    #if def DEBUG_EXPAND_DL
    cprintf("Initial m=%p s=%d 4k=%02X\n\r", memptr, memsize, next_4k);
    #endif

    // Count divs
    for(i = 0; i < dl->lines; i++)
    {
        memptr = (byte*)((unsigned)memptr + (unsigned)BYTES_PER_LINE);
        if(memptr >= (byte*)next_4k)
        {
            memptr = (byte*)next_4k;
            ++count;

            memptr_4k = (unsigned)memptr + 4096;
            rem = memptr_4k % 4096;
            next_4k = memptr_4k - rem;
        }


        #ifdef DEBUG_EXPAND_DL
        cprintf("c=%d: %d m=%p s=%d 4k=%02X\n\r", count, i, memptr, memsize, next_4k);
        #endif
    }

    #ifdef DEBUG_EXPAND_DL
    cgetc();
    reset_console();

    cprintf("Count=%d\n\r", count);
    #endif

    // Allocate count pointers
    new_dl_defs = calloc(count+1, sizeof(dl_def*));
    //memset(new_dl_defs, 0x0, sizeof(dl_def*) * (count+1));

    i = 0;
    while(line_count < dl->lines)
    {
        DLDef* dlist_entry = calloc(1, sizeof(DLDef));
        unsigned rem_lines = dl->lines - line_count;
        unsigned mem_use = rem_lines * BYTES_PER_LINE;

        if(i > 0)
        {
            dlist_entry->blank_lines = 0; 
            dlist_entry->address = next_4k; 
            dlist_entry->dli = 0;
            dlist_entry->mode = dl->mode;
        }
        else
        {
            *dlist_entry = *dl;
        }

        //
        memptr = (byte*)dlist_entry->address;
        memptr_4k = (unsigned)memptr + 4096;
        rem = memptr_4k % 4096;
        next_4k = memptr_4k - rem;

        // Check if memory required crosses 4K boundary.  Compute the number
        // of lines to the 4K boundary.
        if(dlist_entry->address + mem_use > next_4k)
            dlist_entry->lines = (next_4k - dlist_entry->address) / BYTES_PER_LINE;
        else
            dlist_entry->lines = rem_lines;
        
        new_dl_defs[i] = dlist_entry;

        line_count += dlist_entry->lines;

        #ifdef DEBUG_EXPAND_DL
        cprintf("%d (%d): %d, %d, %d, %d, %02X\n\r", i, line_count, new_dl_defs[i]->blank_lines, new_dl_defs[i]->mode, new_dl_defs[i]->lines, new_dl_defs[i]->dli, new_dl_defs[i]->address);
        #endif

        ++i;
    }

    #ifdef DEBUG_EXPAND_DL
    cgetc();
    reset_console();
    #endif

    return new_dl_defs;
}

// Free's all of the memory used by the array.
void cleanupDL_Def_PArray(DLDefParray parray)
{
    DLDef* entry = parray[0];
    byte i = 0;
    while(entry)
    {
        free(entry);

        ++i;

        entry = parray[i];
    }

    free(parray);
}

// Builds a display list that is defined with an array of dl_def's, placing it at dl_location.
// dl_location - location in memory for the DL
// dl[]        - an array of dl_defs that are used to define the modes
// n           - the number of dl_def entries
unsigned makeDisplayList(void* dl_location, DLDef dl[], byte n, struct dl_store* dls)
{
    byte* dl_mem = dl_location;
    int idx;

    // Set new DL
    for(idx = 0; idx < n; idx++)
    {
        int jdx, kdx =0;
        dl_def_parray dl_expanded = expandDisplayList(&dl[idx]);
        dl_def* entry = dl_expanded[kdx];

        while(entry)
        {
            #ifdef DEBUG_EXPAND_DL
            cprintf("%d, %d, %d, %d, %02X\n\r", entry->blank_lines, entry->mode, entry->lines, entry->dli, entry->address);
            #endif
            // Handle blanks, should calculate how many... just using base 8 for now
            for(jdx = 0; jdx < entry->blank_lines/8; jdx++)
                *(dl_mem++) = (byte)DL_BLK8;

            // Handle mode.
            #ifdef DEBUG_EXPAND_DL
            cprintf("l:%d\n\r", entry->lines);
            #endif
            for(jdx = 0; jdx < entry->lines; jdx++)
            {
                if(jdx) // Following line after inital definition which may reference memory
                {
                    if(entry->dli)
                        *(dl_mem++) = DL_DLI(entry->mode);  // DLI flag triggered so generate
                    else
                        *(dl_mem++) = (byte)entry->mode;
                }
                else   // Initial line, may define an LMS
                {
                    if(entry->address)  // Screen memory defined, add address following
                    {
                        if(entry->dli)
                            *(dl_mem++) = DL_DLI(DL_LMS(entry->mode));  // DLI flag triggered so generate
                        else
                            *(dl_mem++) = DL_LMS(entry->mode);

                        *((unsigned*)dl_mem) = entry->address;  // Add the address

                        dl_mem+=2;
                    }
                    else
                    {
                        if(entry->dli)
                            *(dl_mem++) = DL_DLI(entry->mode);  // DLI flag triggered so generate
                        else
                            *(dl_mem++) = (byte)entry->mode;  // Just a plain ol' mode line
                    }
                }
                #ifdef DEBUG_EXPAND_DL
                cprintf("%02X ", *dl_mem);
                #endif
            }

            ++kdx;
            entry = dl_expanded[kdx];
        } // while entry

        cleanupDL_Def_PArray(dl_expanded);
    }

    // Finish up and add jump line
    *(dl_mem++) = DL_JVB;
    *((unsigned*)dl_mem) = (unsigned)dl_location;
    dl_mem+=2;

    dls->size = (unsigned)(dl_mem - (byte*)dl_location);
    dls->mem = dl_location;

    return dls->size;
}

// Shows the contents of a display list.
// name - simply used in the output header so you can tell which DL is which on the console.
// mloc - the location of the DL in memory 
void print_dlist(const char* name, void* mloc)
{
    byte b = 0x00, low, high;
    unsigned idx = 0;

    printf("Displaylist %s at (%p)\n", name, mloc);
    while(1)
    {
        if(idx)
            printf(", ");

        // Get the instruction
        b = ((byte*)mloc)[idx];
        if((b & 0x40) && (b & 0x0F)) // these have two address bytes following)
        {
            low = ((byte*)mloc)[++idx];
            high = ((byte*)mloc)[++idx];
            printf("%02X (%02x%02x)", b, low, high);
        }
        else
            printf("%02X", b);

        idx++;

        if(b == 0x41) // JVB so done... maybe  have to add code to look at the address
            break;
    }
}