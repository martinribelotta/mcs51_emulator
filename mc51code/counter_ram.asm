; Increment A and store to internal RAM 0x30

        ORG 0000h

start:
        MOV A, #00h
loop:
        INC A
        MOV 30h, A
        NOP
        SJMP loop

        END
