; Print Hello & go into infinite loop
mov ah, 0x0e ; tty mode
mov al, 'H'
int 0x10
mov al, 'e'
int 0x10
mov al, 'l'
int 0x10
int 0x10 ; 'l' is still on al
mov al, 'o'
int 0x10

jmp $ ; jump to current address = infinite loop

; Fill with 510 zeros minus the size of the previous code
times 510 - ($-$$) db 0
; Last two bytes are 0x55 and 0xAA
dw 0xAA55



