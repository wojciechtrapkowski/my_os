C_SOURCES = $(wildcard kernel/*.c drivers/*.c cpu/*.c libc/*.c)
HEADERS = $(wildcard kernel/*.h drivers/*.h cpu/*.h libc/*.h)
# Nice syntax for file extension replacement
OBJ = ${C_SOURCES:.c=.o cpu/interrupt.o}

# Change this if your cross-compiler is somewhere else
CC = /usr/local/i386elfgcc/bin/i386-elf-gcc
GDB = /usr/local/i386elfgcc/bin/i386-elf-gdb
# -g: Use debugging symbols in gcc
CFLAGS =  -gdwarf-3 -ffreestanding -Wall -Wextra -fno-exceptions -m32
LDFLAGS = -m elf_i386 -T kernel.ld

# First rule is run by default
os-image.bin: boot/bootsect.bin kernel.bin
	cat $^ > os-image.bin

# '--oformat binary' deletes all symbols as a collateral, so we don't need
# to 'strip' them manually on this case
kernel.bin: boot/kernel_entry.o ${OBJ}
	i386-elf-ld ${LDFLAGS} -o $@ -Ttext 0x1000 $^ --oformat binary

# Used for debugging purposes
kernel.elf: boot/kernel_entry.o ${OBJ}
	i386-elf-ld ${LDFLAGS} -o $@ -Ttext 0x1000 $^ 

# 128 MB
disk.img: os-image.bin
	@if [ ! -f disk.img ]; then \
		dd if=/dev/zero of=disk.img bs=512 count=262144; \
	fi
	@dd if=os-image.bin of=disk.img conv=notrunc bs=512 count=1024
run: os-image.bin disk.img
	qemu-system-i386 -no-reboot -no-shutdown -hda disk.img -monitor stdio 

run_bochs: os-image.bin
	dd if=os-image.bin of=floppy_bochs.img bs=512 conv=sync &
	bochs -f bochsrc.txt
	
# Open the connection to qemu and load our kernel-object file with symbols
debug: os-image.bin disk.img kernel.elf
	qemu-system-i386 -s -hda disk.img -S & 
	${GDB} -ex "target remote localhost:1234" -ex "symbol-file kernel.elf"

# Flag -monitor stdio - means that we can interact with qemu through the terminal
# -d guest_errors,int - means that we want to see the errors in the terminal
# right now hard disk doesnt work, due to the fact that is to small

# Generic rules for wildcards
# To make an object, always compile from its .c
%.o: %.c ${HEADERS}
	${CC} ${CFLAGS} -c $< -o $@

%.o: %.asm
	nasm $< -f elf -o $@

%.bin: %.asm
	nasm $< -f bin -o $@

clean:
	rm -rf *.bin *.dis *.o os-image.bin *.elf
	rm -rf kernel/*.o boot/*.bin drivers/*.o boot/*.o cpu/*.o libc/*.o
	rm -rf disk.img