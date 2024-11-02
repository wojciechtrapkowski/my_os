# Compiling & launching

Compile using nasm:

```nasm -f bin main.asm -o boot_sect_simple.bin```

Launch on emulator:

```qemu-system-x86_64 boot_sect_simple.bin```

Inspect binary

```xxd file.bin```

Compiling

i386-elf-gcc -ffreestanding -c function.c -o function.o

Examining generated assembly code

i386-elf-objdump -d function.o

Checking byte code

xxd file

Linking

i386-elf-ld -o function.bin -Ttext 0x0 --oformat binary function.o

Disassembling

ndisasm -b 32 function.bin

Creating disk image

bximage

or 

qemu-img create -f raw disk.img 100M