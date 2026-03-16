; Simple P1.0 toggle using SETB/CLR and SJMP

        ORG 0000h

start:
        SETB 090h       ; P1.0 bit address
        NOP
        NOP
        NOP
        CLR 090h
        NOP
        NOP
        NOP
        SJMP start

        END
