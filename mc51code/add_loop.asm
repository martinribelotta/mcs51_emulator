; Add immediate to A and mirror to SFR P1 (0x90)

        ORG 0000h

start:
        MOV A, #01h
loop:
        ADD A, #01h
        MOV 90h, A
        SJMP loop

        END
