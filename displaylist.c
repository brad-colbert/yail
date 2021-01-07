// Copyright (C) 2021 Brad Colbert

#include "displaylist.h"

#include <atari.h>

#include <stdio.h>
#include <string.h>

// Builds a display list that is defined with an array of dl_def's, placing it at dl_location.
// dl_location - location in memory for the DL
// dl_def[]    - an array of dl_defs that are used to define the modes
// n           - the number of dl_def entries
unsigned makeDisplayList(void* dl_location, struct dl_def dl[], byte n)
{
    byte* dl_mem = dl_location;
    int idx;

    // Wipe the DL memory
    //memset(dl_mem, 0x0, 0x400); // 1K work

    // Set new DL
    for(idx = 0; idx < n; idx++)
    {
        int jdx;
        struct dl_def* entry = &dl[idx];

        // Handle blanks, should calculate how many... just using base 8 for now
        for(jdx = 0; jdx < entry->blank_lines/8; jdx++)
            *(dl_mem++) = (byte)DL_BLK8;

        // Now handle mode.  No memory, just mode line, else DL_LMS
        for(jdx = 0; jdx < entry->lines; jdx++)
        {
            if(jdx)
            {
                if(entry->dli)
                    *(dl_mem++) = DL_DLI(entry->mode);
                else
                    *(dl_mem++) = (byte)entry->mode;
            }
            else
            {
                if(entry->address)
                {
                    if(entry->dli)
                        *(dl_mem++) = DL_DLI(DL_LMS(entry->mode));
                    else
                        *(dl_mem++) = DL_LMS(entry->mode);
                    *(((unsigned*)dl_mem)++) = entry->address;
                }
                else
                {
                    if(entry->dli)
                        *(dl_mem++) = DL_DLI(entry->mode);
                    else
                        *(dl_mem++) = (byte)entry->mode;
                }
            }
        }
    }

    // Finish up and add jump line
    *(dl_mem++) = DL_JVB;
    *(((unsigned*)dl_mem)++) = (unsigned)dl_location;

    return (unsigned)(dl_mem - (byte*)dl_location);
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