; Global offset
[org 0x7c00]

; will work
mov ah, 0x0e ; tty mode
mov al, [the_secret]
int 10h

; wont work - we will print offset
mov al, the_secret
int 10h

; wont work
mov bx, the_secret
mov al, [bx]
int 0x10

jmp $ ; jump to current address = infinite loop

the_secret:
    db 'X'

; Fill with 510 zeros minus the size of the previous code
times 510 - ($-$$) db 0
; Last two bytes are 0x55 and 0xAA
dw 0xAA55



