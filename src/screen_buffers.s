; Copyright (C) 2021 Brad Colbert

.export _screen_red
.segment "SCREEN_R"
_screen_red:
    .byte $00
    ;.res $22B0, $11

;.export _screen_green
;.segment "SCREEN_G"
;_screen_green:
;    .byte $00
;    ;.res $22B0, $33
;
;.export _screen_blue
;.segment "SCREEN_B"
;_screen_blue:
;    .byte $00
;    ;.res $22B0, $55

; Segment locations
.segment "DATA"
.export _red_buff_segs
_red_buff_segs:
    .byte $00, $80, $00, $90, $00, $A0

;.export _green_buff_segs
;_green_buff_segs:
;    .byte $B0, $72, $00, $80, $00, $90
;
;.export _blue_buff_segs
;_blue_buff_segs:
;    .byte $60, $95, $00, $A0, $00, $B0

; Segment sizes
.export _red_buff_sizes
_red_buff_sizes:
    .byte $F0, $0F, $F0, $0F, $80, $02

;.export _green_buff_sizes
;_green_buff_sizes:
;    .byte $48, $0D, $F0, $0F, $28, $05
;
;.export _blue_buff_sizes
;_blue_buff_sizes:
;    .byte $A0, $0A, $F0, $0F, $D0, $07