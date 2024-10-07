[org 0x7c00] ; tell the assembler that our offset is bootsector code

mov bx, 0x9000 ; es:bx = 0x0000:0x9000 = 0x09000
mov dh, 2 ; read 2 sectors
call disk_load

mov dx, [0x9000] ; first word from first loaded sector, 0xdada
call print_hex

call print_nl

mov dx, [0x9000 + 512] ; first word from second loaded sector, 0xface
call print_hex

; that's it! we can hang now
jmp $

; remember to include subroutines below the hang
%include "boot_sect_print.asm"
%include "boot_sect_print_hex.asm"
%include "boot_sect_disk.asm"

; data
HELLO:
    db 'Hello, World', 0

GOODBYE:
    db 'Goodbye', 0

; padding and magic number
times 510-($-$$) db 0
dw 0xaa55

; boot sector 1
times 256 dw 0xdada ; boot sector 2
times 256 dw 0xface ; boot sector 3