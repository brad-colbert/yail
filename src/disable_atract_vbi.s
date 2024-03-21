;; Simple VBI routine to disable the attract mode

.export _add_attract_disable_vbi
.export _remove_attract_disable_vbi

.import _ORIG_VBII_SAVE

.segment "CODE"

;; An interupt routine for the VBI
SETVBV = $E45C
XITVBV = $E462
SYSVBV = $E45F
ATRACT = $4D

; Disable ATRACT mode by setting the "register" to zero
.PROC disable_attract
        pha
        lda         #0
        sta         ATRACT
        pla
        jmp         SYSVBV
.ENDPROC

_add_attract_disable_vbi:

        lda         #6                ; Immediate mode
        ldy         #<disable_attract  ; high byte of address
        ldx         #>disable_attract  ; low byte of address
        jsr         SETVBV

        rts

_remove_attract_disable_vbi:

        lda         #6                ; Immediate mode
        ldy         _ORIG_VBII_SAVE  ; high byte of address
        ldx         _ORIG_VBII_SAVE+1  ; low byte of address
        jsr         SETVBV

        rts

