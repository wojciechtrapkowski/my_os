#include "../cpu/isr.h"
#include "../cpu/timer.h"
#include "../drivers/keyboard.h"
#include "../drivers/screen.h"

#include <stdint.h>

void kernel_main() {
    clear_screen();
    isr_install();
    irq_install();    
    kprepare_space_for_info();

    kprint("Type something, it will go through the kernel\n"
        "Type END to halt the CPU\n> ");
}

void user_input(char *input) {
    if (strcmp(input, "END") == 0) {
        kprint("Stopping the CPU. Bye!\n");
        asm volatile("hlt");
    } else if (strcmp(input, "PAGE") == 0) {
        // This should result in a page fault
        uint32_t* ptr = (uint32_t*)0xA0000000; 
        *ptr = 123;

     
    }
    kprint("You said: ");
    kprint(input);
    kprint("\n> ");
}