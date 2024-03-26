.include  "atari.inc"

.export _wait_vbi

.proc _wait_vbi
    lda RTCLOK+2
:   cmp RTCLOK+2
    beq :-
    rts
.endproc

