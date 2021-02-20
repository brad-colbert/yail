; Copyright (C) 2021 Brad Colbert

.export _screen_red
.segment "SCREEN_R"
_screen_red:
    .byte $00
    ;.res $22B0, $11

.export _screen_green
.segment "SCREEN_G"
_screen_green:
    .byte $00
    ;.res $22B0, $33

.export _screen_blue
.segment "SCREEN_B"
_screen_blue:
    .byte $00
    ;.res $22B0, $55

; Segment locations
.segment "DATA"
.export _red_buff_segs
_red_buff_segs:
    .byte $00, $51, $00, $60, $00, $70

.export _green_buff_segs
_green_buff_segs:
    .byte $B0, $74, $00, $80, $00, $90

.export _blue_buff_segs
_blue_buff_segs:
    .byte $60, $98, $00, $A0, $00, $B0

; Segment sizes
.export _red_buff_sizes
_red_buff_sizes:
    .byte $00, $0F, $F0, $0F, $70, $03

.export _green_buff_sizes
_green_buff_sizes:
    .byte $40, $0B, $F0, $0F, $30, $07

.export _blue_buff_sizes
_blue_buff_sizes:
    .byte $80, $07, $F0, $0F, $F0, $0A