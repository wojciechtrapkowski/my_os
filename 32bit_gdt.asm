gdt_start:
    ; GDT starts with a null 8-byte
    ; as Intel Manual says
    dd 0x0
    dd 0x0

; GDT for code segment base = 0x00000000, length = 0xfffff
gdt_code:
    ; page 100 in intel manual

    ; first part - 4 bytes
    dw 0xffff ; segment length bits 0-15
    dw 0x0 ; segment base bits 0-15

    ; second part - another 4 bytes
    db 0x0 ; segment base bits 16-23
    db 10011010b ; flags (8 bits)
    ; type - 1010:
    ;   - 1 - for code, since this is code segment
    ;   - 0 - conforming - by setting it to 0, code with lower privilege may not call code in this segment - memory protection
    ;   - 1 - readable - it allows us to read constants in the code
    ;   - 0 - accessed - for example for debugging
    ; descriptor type 
    ;   - 1 - code or data
    ;   - 0 - system
    ; descriptor privilege level
    ;   - 0 - highest one
    
    db 11001111b ; flags (4 bits) + segment length, bits 16-19
    ; segment present
    ;   - 1 because it is present in memory - for virtual memory
    ; segment limit 3 bits
    ; available for use by system software
    ;   - 0 - we can set it for our own use, but right now it is not needed
    ; 64-bit code segment 1 bit - 0
    ; default operation size - 1
    ; granularity - 1
    ;   - when set it allows our segment to span to 4 GB - multiply by 4 K - shift 3 hex digits to the left
    db 0x0 ; segment base, bits 14-21

; GDT for data segment. base and length identical to code segment - they will overlap
gdt_data:
    dw 0xffff
    dw 0x0
    db 0x0
    ; slightly different flags
    db 10010010b
    db 11001111b
    db 0x0

gdt_end:

; GDT descriptor
gdt_descriptor:
    dw gdt_end - gdt_start - 1 ; size (16 bit), always one less of its true size
    dd gdt_start ; address (32 bit)

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start