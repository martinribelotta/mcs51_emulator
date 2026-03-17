            ORG 0000h
            LJMP start

            ORG 0030h
start:      MOV SP, #70h
            MOV TMOD, #20h      ; Timer1 mode 2 (8-bit auto-reload)
            MOV TH1, #0FAh      ; ~10416 baud @ 12MHz with SMOD=1
            MOV TL1, #0FAh
            MOV PCON, #80h      ; SMOD=1
            MOV SCON, #50h      ; Mode 1, REN=1
            SETB TR1

            MOV DPTR, #msg
send_loop:  CLR A
            MOVC A, @A+DPTR
            JZ done
            ACALL putc
            INC DPTR
            SJMP send_loop

done:       SJMP done

putc:       MOV SBUF, A
wait_ti:    JNB TI, wait_ti
            CLR TI
            RET

msg:        DB 'Hello world', 0Dh, 0Ah, 00h
            END
