#include "console.h"
#include "types.h"

// Globals
byte console_state = CONSOLE_HIDDEN;

void enable_console(void)
{
    console_state = CONSOLE_SHOWN;
}

void disable_console(void)
{
    console_state = CONSOLE_HIDDEN;
}