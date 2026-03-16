; Simple ACALL/RET test: increments A inside subroutine

        ORG 0000h

start:
        MOV A, #00h
        ACALL inc_sub
        ACALL inc_sub
        SJMP start

inc_sub:
        INC A
        RET

        END
