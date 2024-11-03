#include "../cpu/isr.h"
#include "../cpu/timer.h"
#include "../drivers/keyboard.h"
#include "../drivers/screen.h"
#include "../drivers/disk.h"
#include <stdint.h>
#include "heap.h"

KHEAP_T kheap;

void kernel_main() {

    clear_screen();
    isr_install();
    irq_install();    
    kprepare_space_for_info();

    kinit_heap();                                                          

    kprint("Type something, it will go through the kernel\n"
        "Type END to halt the CPU\n> ");
}

void user_input(char *input) {
    if (strcmp(input, "END") == 0) {
        kprint("Stopping the CPU. Bye!\n");
        asm volatile("hlt");
    } else if (strcmp(input, "PAGE") == 0) {
        // This should result in a page fault
        // but should be handled by the kernel
        uint32_t* ptr = (uint32_t*)0xA0000000;
        *ptr = 123;
    } else if (strcmp(input, "DISKREAD") == 0) {
        uint32_t* ptr = (uint32_t*)0xFFFF;
        *ptr = 123;
        kprint_hex(*ptr);
        ata_dma_read(0, 0, 1, (void*)0xFFFF);
    } else if (strcmp(input, "DISKWRITE") == 0) {
        uint32_t* ptr = (uint32_t*)0xFFFF;
        *ptr = 123;
        ata_dma_write(0, 0, 1, (void*)0xFFFF);
    } else if (strcmp(input, "HEAP") == 0) {
        uint32_t* ptr;
        char* ptr2;

        ptr = (uint32_t*)kmalloc(256);
        kprint("Allocated 256 bytes\n");
        kprint_hex((uint32_t)ptr);
        kprint("\n");

        ptr2 = (char*)kmalloc(256);
        kprint("Allocated another 256 bytes\n");
        kprint_hex((uint32_t)ptr2);
        kprint("\n");
            
        kfree(ptr);                      
        kfree(ptr2);                     
        
        kprint("Freed 256 * 2 bytes\n");
    } else {
        kprint("You said: ");
        kprint(input);
        kprint("\n> ");
    }
}