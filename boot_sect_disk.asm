disk_load:
    pusha
    
    ; we will overwrite this register
    ; so store it to retrieve parameter
    ; later
    push dx

    mov ah, 0x02 ; 0x02 - read function
    mov al, dh ; number of sectors to read
    mov cl, 0x02 ; sector to start from, first one is our boot sector, so skip it
    mov ch, 0x00 ; cylinder
    mov dh, 0x00 ; head 

    ; The BIOS sets dl to the drive number before calling the bootloader
    ; es:bx - pointer to buffer, where data will be stored

    int 0x13
    jc disk_error

    pop dx
    cmp al, dh
    jne sectors_error
    popa
    ret

disk_error:
    mov bx, DISK_ERROR
    call print
    call print_nl
    mov dh, ah ; ah = error code, dl = disk drive that dropped the error
    call print_hex
    jmp disk_loop

sectors_error:
    mov bx, SECTORS_ERROR
    call print

disk_loop:
    jmp $


DISK_ERROR: db "Disk read error", 0
SECTORS_ERROR: db "Incorrect number of sectors read", 0