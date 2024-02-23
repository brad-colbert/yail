// Copyright (C) 2021 Brad Colbert

#include "utility.h"
#include "types.h"

#include <conio.h>
#include <stdbool.h>

void pause(const char* message)
{
    if(message)
        cputs(message);

    while(true)
    {
        if(kbhit())
        {
            cgetc();  // flush the key press
            break;
        }
    }
}