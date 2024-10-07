print:
    pusha

start:
    ; bx - base address of string
    mov al, [bx]
    cmp al, 0
    je done

    ; print
    mov ah, 0x0e
    int 0x10

    add bx, 1
    jmp start

done:
    popa
    ret

; print new line character
print_nl:
    pusha

    mov ah, 0x0e
    mov al, 0x0a ; newline char
    int 0x10
    mov al, 0x0d ; carriage return
    int 0x10

    popa
    ret