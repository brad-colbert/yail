// Copyright (C) 2021 Brad Colbert

#ifndef DISPLAY_LISTS_H
#define DISPLAY_LISTS_H

#include "types.h"

#pragma bss-name (push,"DLIST_R")  
extern byte dlist_red[]; 
#pragma bss-name (pop)
#pragma bss-name (push,"DLIST_G")  
extern byte dlist_green[]; 
#pragma bss-name (pop)
#pragma bss-name (push,"DLIST_B")  
extern byte dlist_blue[]; 
#pragma bss-name (pop)

#endif // DISPLAY_LISTS_H