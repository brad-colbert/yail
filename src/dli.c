// Copyright (C) 2021 Brad Colbert

#include "screen_buffers.h"
#include "display_lists.h"
#include "types.h"

#include <peekpoke.h>
#include <atari.h>

#pragma optimize(push, off)
void render_green(void);
void render_blue(void);

void render_red(void) {
    __asm__("pha");
    __asm__("tya");
    __asm__("pha");
    __asm__("txa");
    __asm__("pha");
    OS.color4 = COLOR_RED;
    OS.vdslst = render_green;
//    OS.sdlst = dlist_green;
    __asm__("sta %w", 0xD40A);
    //ANTIC.wsync = 0xFF;
    __asm__("pla");
    __asm__("tax");
    __asm__("pla");
    __asm__("tay");
    __asm__("pla");
    __asm__("rti");
}

void render_green(void) {
    __asm__("pha");
    __asm__("tya");
    __asm__("pha");
    __asm__("txa");
    __asm__("pha");
    OS.color4 = COLOR_GREEN;
    OS.vdslst = render_blue;
//    OS.sdlst = dlist_blue;
    __asm__("sta %w", 0xD40A);
    __asm__("pla");
    __asm__("tax");
    __asm__("pla");
    __asm__("tay");
    __asm__("pla");
    __asm__("rti");
}

void render_blue(void) {
    __asm__("pha");
    __asm__("tya");
    __asm__("pha");
    __asm__("txa");
    __asm__("pha");
    OS.color4 = 128;
    OS.vdslst = render_red;
//    OS.sdlst = dlist_red;
    __asm__("sta %w", 0xD40A);
    __asm__("pla");
    __asm__("tax");
    __asm__("pla");
    __asm__("tay");
    __asm__("pla");
    __asm__("rti");
}
#pragma optimize(pop)
