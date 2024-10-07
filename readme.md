# Compiling & launching

Compile using nasm:

```nasm -f bin main.asm -o boot_sect_simple.bin```

Launch on emulator:

```qemu-system-x86_64 boot_sect_simple.bin```

Inspect binary

```xxd file.bin```
