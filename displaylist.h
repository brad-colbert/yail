// Copyright (C) 2021 Brad Colbert

#ifndef DISPLAY_LIST_H
#define DISPLAY_LIST_H

#include "files.h"
#include "types.h"
#include "graphics.h"

// Inspects a dl_def and determines if the memory use crosses 4K boundaries.
// If it does, it breaks the definition up, creating new entries at the 4K
// boundaries.
dl_def_parray expandDisplayList(DLDef* dl);

// Free's all of the memory used by the array.
void cleanupDL_Def_PArray(DLDefParray parray);

// Builds a display list that is defined with an array of dl_def's, placing it at dl_location.
// dl_location - location in memory for the DL
// dl_def[]    - an array of dl_defs that are used to define the modes
// n           - the number of dl_def entries
unsigned makeDisplayList(void* dl_location, dl_def dl[], byte n, struct dl_store* dls);

// Shows the contents of a display list.
// name - simply used in the output header so you can tell which DL is which on the console.
// mloc - the location of the DL in memory 
void print_dlist(const char* name, void* mloc);

#endif // DISPLAY_LIST_H