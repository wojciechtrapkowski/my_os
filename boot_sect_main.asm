[org 0x7c00] ; tell the assembler that our offset is bootsector code

mov bx, 0x9000 ; es:bx = 0x0000:0x9000 = 0x09000
mov sp, bp

mov bx, MSG_REAL_MODE
call print ; This will be written after the BIOS messages

call switch_to_pm
jmp $ ; this will actually never be executed


%include "boot_sect_print.asm"
; %include "boot_sect_print_hex.asm"
; %include "boot_sect_disk.asm"

%include "32bit_gdt.asm"
%include "32bit_print.asm"
%include "32bit_switch.asm"

[bits 32]
BEGIN_PM: ; after the switch we will get here
    mov ebx, MSG_PROT_MODE
    call print_string_pm
    jmp $

MSG_REAL_MODE db "Started in 16-bit real mode", 0
MSG_PROT_MODE db "Loaded 32-bit protected mode", 0

; bootsector 0 
times 510-($-$$) db 0
dw 0xaa55

; ; boot sector 1
; times 256 dw 0xdada ; boot sector 2
; times 256 dw 0xface ; boot sector 3