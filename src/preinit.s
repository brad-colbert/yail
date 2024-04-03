        .export     pre_init
        .include    "atari.inc"
;        .include    "macros.inc"

; https://github.com/markjfisher/fujinet-config-ng/blob/master/src/atari/common/init/pre_init.s

; this segment is loaded during disk load before dlist and main are loaded
; the memory location will be written over by later blocks, which is fine, it's only needed for one time initial setup
.segment "INIT"
.proc pre_init
        lda     #$c0        ; check if ramtop is already ok
        cmp     RAMTOP
        beq     ramok
        sta     RAMTOP      ; set ramtop to end of basic
        sta     RAMSIZ      ; and ramsiz too

        lda     PORTB
        ora     #$02        ; disable basic bit
        sta     PORTB

        lda     #$01        ; keep it off after reset
        sta     BASICF

        ldx     #$02        ; CLOSE "E"
        jsr     editor
        ldx     #$00        ; OPEN "E"
editor:
        ; dispatch based JMP!
        lda     EDITRV+1, x
        pha
        lda     EDITRV, x
        pha
        ; now ready to JMP on a RTS
ramok:
        rts

clear_from:
.endproc