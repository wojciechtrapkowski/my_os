[org 0x7c00] ; tell the assembler that our offset is bootsector code
KERNEL_OFFSET equ 0x1000 ; this needs to be the same as in linking process

    mov [BOOT_DRIVE], dl ; BIOS sets us the boot drive in 'dl' on the boot
    mov bx, 0x9000 ; es:bx = 0x0000:0x9000 = 0x09000
    mov sp, bp

    mov bx, MSG_REAL_MODE
    call print ; This will be written after the BIOS messages
    call print_nl

    call load_kernel ; read the kernel from disk
    call switch_to_pm ; disable interrupts, load GDT,  etc. Finally jumps to 'BEGIN_PM'
    
    jmp $ ; this will actually never be executed


%include "boot_sect_print.asm"
%include "boot_sect_print_hex.asm"
%include "boot_sect_disk.asm"
%include "32bit_gdt.asm"
%include "32bit_print.asm"
%include "32bit_switch.asm"

[bits 16]
load_kernel:
    mov bx, MSG_LOAD_KERNEL
    call print
    call print_nl

    mov bx, KERNEL_OFFSET
    mov dh, 2 ; load 2 sectors
    mov dl, [BOOT_DRIVE]
    call disk_load
    ret
    

[bits 32]
BEGIN_PM: ; after the switch we will get here
    mov ebx, MSG_PROT_MODE
    call print_string_pm
    call KERNEL_OFFSET ; jump to kernel code
    jmp $

BOOT_DRIVE db 0 
MSG_REAL_MODE db "Started in 16-bit real mode", 0
MSG_PROT_MODE db "Loaded 32-bit protected mode", 0
MSG_LOAD_KERNEL db "Loading kernel", 0

; bootsector 0 
times 510-($-$$) db 0
dw 0xaa55

; ; boot sector 1
; times 256 dw 0xdada ; boot sector 2
; times 256 dw 0xface ; boot sector 3