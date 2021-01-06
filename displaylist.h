#ifndef DISPLAY_LIST_H
#define DISPLAY_LIST_H

#include "types.h"

// A simple structure for defining a display list in a code compact way
struct dl_def
{
    byte blank_lines;
    byte mode;    // From the Antic modes
    byte lines;   // # of lines of the mode
    unsigned address; // Address of screen memory for mode, 0x0000 if use SAVMSC + offset
    byte dli; // Switch for DLI
};

// Builds a display list that is defined with an array of dl_def's, placing it at dl_location.
// dl_location - location in memory for the DL
// dl_def[]    - an array of dl_defs that are used to define the modes
// n           - the number of dl_def entries
unsigned makeDisplayList(void* dl_location, struct dl_def dl[], byte n);

// Shows the contents of a display list.
// name - simply used in the output header so you can tell which DL is which on the console.
// mloc - the location of the DL in memory 
void print_dlist(const char* name, void* mloc);

#endif // DISPLAY_LIST_H