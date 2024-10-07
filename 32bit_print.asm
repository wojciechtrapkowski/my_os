[bits 32]

VIDEO_MEMORY equ 0xb8000
WHITE_ON_BLACK equ 0x0f ; the color byte for each character

print_string_pm:
    pusha
    mov edx, VIDEO_MEMORY

print_string_pm_loop:
    mov al, [ebx] ; ebx is address of our character
    mov ah, WHITE_ON_BLACK

    cmp al, 0
    je print_string_pm_done

    mov [edx], ax ; store character & attribute in the memory
    add ebx, 1 ; next character
    add edx, 2 ; next position in memory

    jmp print_string_pm_loop

print_string_pm_done:
    popa
    ret