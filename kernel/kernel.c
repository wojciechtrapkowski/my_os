#include "../cpu/isr.h"
#include "../cpu/timer.h"
#include "../cpu/paging.h"
#include "../cpu/physical_memory_manager.h"
#include "../drivers/keyboard.h"
#include "../drivers/screen.h"
#include "../drivers/disk.h"
#include <stdint.h>
#include "heap.h"
#include "fs.h"

extern uint32_t _kernel_start;
extern uint32_t _kernel_end;

void print_help() {
    kprint("\nAvailable Commands:\n");
    kprint("------------------\n");
    kprint("Memory Tests:\n");
    kprint("  PAGE     - Test page fault handling\n");
    kprint("  HEAP     - Test heap allocation\n");
    kprint("  SWAP     - Test memory swapping\n");
    kprint("\nFile System:\n");
    kprint("  MKDIR    - Create test directory\n");
    kprint("  MKFILE   - Create test file\n");
    kprint("  LIST     - Show directory contents\n");
    kprint("\nSystem:\n");
    kprint("  HELP     - Show this menu\n");
    kprint("  END      - Halt system\n\n");
}

void kernel_main() {
    clear_screen();
    kprint("OS Kernel Initializing...\n");

    kprint("Setting up interrupts...\n");
    isr_install();
    irq_install();

    kprint("Initializing memory management...\n");
    init_physical_memory_manager();
    init_paging();
    init_heap();
    init_filesystem();

    // Print kernel info
    kprint("\nKernel Information:\n");
    kprint("Start Address: 0x"); kprint_hex((uint32_t)&_kernel_start);
    kprint("\nEnd Address: 0x");  kprint_hex((uint32_t)&_kernel_end);
    kprint("\nTotal Size: ");
    kprint_int((uint32_t)(&_kernel_end - &_kernel_start));
    kprint(" bytes\n");

    kprint("\nSystem ready! Type 'HELP' for commands\n> ");
}

void user_input(char *input) {
    if (strcmp(input, "HELP") == 0) {
        print_help();
    }
    else if (strcmp(input, "END") == 0) {
        kprint("Saving file system...\n");
        fs_save();
        kprint("System halting...\n");
        while (1) { asm volatile("nop"); }
    }
    else if (strcmp(input, "PAGE") == 0) {
        kprint("Testing page fault handler...\n");
        uint8_t* ptr = (uint8_t*)0xA0000000;
        *ptr = 123;
        kprint("Page fault handler test complete\n");
    }
    else if (strcmp(input, "HEAP") == 0) {
        kprint("Testing heap allocation...\n");
        
        char* ptr = (char*)kmalloc(256);
        if (ptr == NULL) {
            kprint("Failed to allocate memory\n");
            return;
        }
        kprint("Allocated 256 bytes at ");
        kprint_hex((uint32_t)ptr);
        
        char* ptr2 = (char*)kmalloc(256);
        if (ptr2 == NULL) {
            kprint("Failed to allocate memory\n");
            return;
        }
        kprint("\nAllocated 256 bytes at ");
        kprint_hex((uint32_t)ptr2);
        
        kprint("\nFreeing memory...\n");
        kfree(ptr);
        kfree(ptr2);
        kprint("Heap test complete\n");
    }
    else if (strcmp(input, "SWAP") == 0) {
        kprint("Testing memory swapping...\n");
        for (uint32_t i = 0; i < 40; i++) {
            char* ptr = (char*)(0x17346000 + (i * 4096));
            *ptr = 'A';
            kprint("Allocated page "); 
            kprint_int(i); 
            kprint("\n");
            
            // Delay to see swapping in action
            for (uint32_t j = 0; j < 10000000; j++) {
                asm volatile("nop");
            }
        }
        kprint("Reading first page: ");
        char* ptr = (char*)0x17346000;
        kprint(ptr);
        kprint("\nSwap test complete\n");
    }
    else if (strcmp(input, "MKDIR") == 0) {
        kprint("Creating directory 'test'...\n");
        fs_create_directory(NULL, "test");
        kprint("Directory created\n");
    }
    else if (strcmp(input, "MKFILE") == 0) {
        kprint("Creating file 'test'...\n");
        fs_create_file(NULL, "test");
        kprint("File created\n");
    }
    else if (strcmp(input, "LIST") == 0) {
        kprint("Directory contents:\n");
        fs_list_directory(NULL);
    }
    else {
        kprint("Unknown command. Type 'HELP' for available commands\n");
    }
    kprint("> ");
}