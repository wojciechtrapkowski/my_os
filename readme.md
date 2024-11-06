# My Operating System

## Overview
This is a custom very simple operating system implementation to learn how it works under the hood.

## Implementation Progress

- [x] Bootloader
  - [x] Basic bootloader
  - [x] Switch to protected mode
  - [x] Load kernel from disk
- [x] GDT Table
  - [x] Code & data segment for kernel
- [x] Interrupts
  - [x] Set up ISR
  - [x] Remap PIC & set up interrupts for IRQ
    - [x] Timer
    - [x] Keyboard
    - [x] RTC - Date & Time
    - [x] Disk Driver - ATA with DMA
- [x] Memory Management
  - [x] Physical memory manager
  - [x] Paging implementation
    - [x] Map whole QEMU memory
  - [x] Page fault handler
  - [x] Memory swapping
  - [x] Heap implementation
- [ ] File system
    - note: implemented basic version.
- [ ] System calls - API
- [ ] Enter user mode
    - [ ] Shell
    - [ ] Basic commands
    - [ ] User programs

## Memory layout

### Physical Memory (RAM) + Virtual Memory
```
0x00000000 - 0x00000400    1KB     Reserved (Real Mode)
0x00001000 - 0x001FFFFF    ~2MB    Kernel
    0x00001000             --      Kernel Start
    0x00010000             --      Physical Memory Allocator
    0x000F0000             --      Kernel Heap Start - Virtual Memory
    0x001FFFFF             --      Kernel End - Virtual Memory
0x08000000                 --      End of Physical Memory (128MB) - it is fully mapped
```

### Disk Layout
```
Sector 0                   512B    MBR (Boot Sector)
0x00000200 - 0x001FFFFF    ~2MB    Kernel Storage
0x00200000 - 0x01200000    16MB    Page Swap Space
0x01200000 - 0x02200000    16MB    File System Metadata
0x02200000 - EOF           ~66MB   File Storage
```

## Programs used
- NASM (Netwide Assembler)
- QEMU emulator
- Bochs emulator
- gcc cross-compiler
- ndisasm (for disassembly)
- xxd (for hex dumps)

## Using tools

### Compiling Assembly Code
```bash
# Compile assembly to binary
nasm -f bin main.asm -o boot_sect_simple.bin
```

### Compiling C Code
```bash
# Compile C code to object file
i386-elf-gcc -ffreestanding -c function.c -o function.o

# Link object file to binary
i386-elf-ld -o function.bin -Ttext 0x0 --oformat binary function.o
```

### Creating Disk Image
Using bximage:
```bash
bximage
```

Or using QEMU:
```bash
qemu-img create -f raw disk.img 100M
```

### Debugging and Analysis Tools

#### Examine Binary Content
```bash
# View hex dump of binary file
xxd file.bin
```

#### View Assembly Code
```bash
# Examine generated assembly
i386-elf-objdump -d function.o

# Disassemble binary
ndisasm -b 32 function.bin
```

## Project Structure
```
├── boot/
│   ├── boot_sect.asm          # Main bootloader
│   ├── disk.asm               # BIOS Disk routines
│   ├── gdt.asm                # Global Descriptor Table
│   └── switch_pm.asm          # Mode switching utilities
│
├── cpu/
│   ├── idt.c                 # Interrupt Descriptor Table
│   ├── idt.h
│   ├── isr.c                 # Interrupt Service Routines
│   ├── isr.h
│   ├── timer.c               # PIT timer
│   ├── timer.h
│   ├── ports.c               # I/O port operations
│   └── ports.h
│
├── drivers/
│   ├── keyboard.c            # Keyboard driver
│   ├── keyboard.h
│   ├── screen.c              # Screen output
│   ├── screen.h
│   ├── disk.c                # ATA disk driver with DMA
│   ├── disk.h
│   ├── rtc.c                 # Real-time clock
│   └── rtc.h
│
├── kernel/
│   ├── kernel.c              # Main kernel file
│   ├── kernel.h
│   └── memory/
│       ├── paging.c          # Paging implementation
│       ├── paging.h
│       ├── physical_memory_manager.c
│       ├── physical_memory_manager.h
│       ├── heap.c            # Kernel heap
│       ├── heap.h
│       ├── memory_consts.h   # Memory constants
│
├── libc/
│   ├── string.c             # String operations
│   ├── string.h
│   ├── mem.c                # Memory operations
│   └── mem.h
│
├── Makefile                 # Build system
├── linker.ld                # Linker script
└── README.md                # Project documentation
```