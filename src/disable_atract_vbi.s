;; Simple VBI routine to disable the attract mode

.export _add_attract_disable_vbi
.export disable_atract

.segment "DATA"

.segment "CODE"

;; An interupt routine for the VBI
SETVBV = $E45C
XITVBV = $E462
SYSVBV = $E45F
ATRACT = $4D

; Disable ATRACT mode by setting the "register" to zero
.PROC disable_atract
        pha
        lda         #0
        sta         ATRACT
        pla
        jmp         SYSVBV
.ENDPROC

_add_attract_disable_vbi:

        lda         #6                ; Immediate mode
        ldy         #<disable_atract  ; hight byte of address
        ldx         #>disable_atract  ; low byte of address
        jsr         SETVBV

        rts

