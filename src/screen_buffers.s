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
