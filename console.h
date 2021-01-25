// Copyright (C) 2021 Brad Colbert

#ifndef CONSOLE_H
#define CONSOLE_H

//
#define CONSOLE_HIDDEN 0
#define CONSOLE_SHOWN 1

void enable_console(void);
void disable_console(void);
void console_update(void);
void reset_console(void);
void fix_chars(char*);

#endif // CONSOLE_H