#include "../cpu/isr.h"
#include "../cpu/timer.h"
#include "../cpu/paging.h"
#include "../drivers/keyboard.h"
#include "../drivers/screen.h"
#include "../drivers/disk.h"
#include <stdint.h>
#include "heap.h"
#include "fs.h"
KHEAP_T kheap;

// In order to set up heaps we need to know where the kernel starts and ends
extern uint32_t _kernel_start;
extern uint32_t _kernel_end;  

void kernel_main() {
    clear_screen();

    kprint("Initializing kernel...\n");
    // kprepare_space_for_info(); // This will be used later

    kprint("Installing ISRs...\n");
    isr_install();
    kprint("Installing IRQs...\n");
    irq_install();    

    kprint("Initializing heaps...\n");
    init_heap();                   
    // kprint("Initializing filesystem...\n");
    // init_filesystem();
    kprint("Initializing paging...\n");
    init_paging();

    kprint("Kernel start: ");
    kprint_hex((uint32_t)&_kernel_start);
    kprint("\nKernel end: ");
    kprint_hex((uint32_t)&_kernel_end);
    kprint("\nKernel size: ");
    char num[16];
    int_to_ascii((uint32_t)(&_kernel_end - &_kernel_start), num);
    kprint(num);
    kprint(" bytes\n\n");

    kprint("Type something, it will go through the kernel\n"
        "Type END to halt the CPU\n> ");
}

void user_input(char *input) {
    if (strcmp(input, "END") == 0) {
        kprint("Stopping the CPU. Bye!\n");
        fs_save();
        // Wait for interrupt and then halt
        // asm volatile("hlt");
        while (1) {
            asm volatile("nop");
        }
    } else if (strcmp(input, "PAGE") == 0) {
        // This should result in a page fault
        // but should be handled by the kernel
        uint32_t virt_addr = (20 << 22);  // Directory 20, offset 0
    uint8_t* memory = (uint8_t*)virt_addr;
    *memory = 123;

        uint8_t* ptr = (uint8_t*)0xA0000000;
        *ptr = 123;
        paging_test_swap();
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
    } else if (strcmp(input, "MKDIR") == 0) {
        fs_create_directory(NULL, "test");
    } else if (strcmp(input, "MKFILE") == 0) {
        fs_create_file(NULL, "test");
    } else if (strcmp(input, "LIST") == 0) {
        fs_list_directory(NULL);
    } else if (strcmp(input, "SWAP") == 0) {
        // This should trigger a swap

         for (uint32_t i = 0; i < 40; i++) {
            char* ptr = (char*)(0x17346000 + (i * 4096));
            *ptr = 'A';  // This should eventually trigger swapping
            // Add delay
            for (uint32_t j = 0; j < 10000000; j++) {
                asm volatile("nop");
            }
        }
        char* ptr = (char*)0x17346000;
        kprint(ptr);
        kprint("\n");
    } else {
        kprint("You said: ");
        kprint(input);
        kprint("\n> ");
    }
}