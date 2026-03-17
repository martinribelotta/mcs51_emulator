            ORG 0000h
            LJMP start

            ORG 0030h
start:      MOV P1, #00h

main:       MOV P1, #0FFh     ; ON
            ACALL delay
            MOV P1, #00h      ; OFF
            ACALL delay
            SJMP main

; ~250ms delay @ 12MHz (1us per machine cycle).
; Nested loops: 2 * 250 * 250 * (DJNZ cycles) ≈ 250ms.
delay:      MOV R5, #2        ; outermost
d0:         MOV R7, #250      ; outer
d1:         MOV R6, #250      ; inner
d2:         DJNZ R6, d2
            DJNZ R7, d1
            DJNZ R5, d0
            RET

            END
