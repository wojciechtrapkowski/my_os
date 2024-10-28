#include "../cpu/isr.h"
#include "../cpu/timer.h"
#include "../drivers/keyboard.h"
#include "../drivers/screen.h"


int main() {
    clear_screen();
    isr_install();
    irq_install();    
    kprepare_space_for_info();

    kprint("Type something, it will go through the kernel\n"
        "Type END to halt the CPU\n> ");

    return 0;
}

void user_input(char *input) {
    if (strcmp(input, "END") == 0) {
        kprint("Stopping the CPU. Bye!\n");
        asm volatile("hlt");
    } else if (strcmp(input, "PAGE") == 0) {
        u32 phys_addr;
        u32 page = kmalloc(1000, 1, &phys_addr);
        char page_str[16] = "";
        hex_to_ascii(page, page_str);
        char phys_str[16] = "";
        hex_to_ascii(phys_addr, phys_str);
        kprint("Page: ");
        kprint(page_str);
        kprint(", physical address: ");
        kprint(phys_str);
        kprint("\n");
    }
    kprint("You said: ");
    kprint(input);
    kprint("\n> ");
}