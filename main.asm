; Global offset
[org 0x7c00]

; address far away from 0x7c00, so we won't get overwritten
mov bp, 0x8000 ; bp points to the bottom of stack
mov sp, bp ; sp points to the top

push 'A'
push 'B'
push 'C'

mov ah, 0x0e ; tty mode
mov al, [bp-2]
int 10h
mov al, [bp-4]
int 10h
mov al, [bp-6]
int 10h

push 'A'
push 'B'
push 'C'

pop bx
mov al, bl
int 10h

pop bx
mov al, bl
int 10h

pop bx
mov al, bl
int 10h

jmp $ ; jump to current address = infinite loop

; Fill with 510 zeros minus the size of the previous code
times 510 - ($-$$) db 0
; Last two bytes are 0x55 and 0xAA
dw 0xAA55



