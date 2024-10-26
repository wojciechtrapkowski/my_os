#include "../drivers/screen.h"
#include "util.h"
#include "../cpu/isr.h"
#include "../cpu/idt.h"

void test_whole_screen() {
    char* video_memory = (char*)(0xb8000);
    int index = 1;
    for (int i=0; i<25; i += 1) {
        for (int j=0; j<80; j+=1) {
            index = (i * 160) + (j*2);
           *(video_memory + index) = 'Y';
           *(video_memory + index + 1) = 0x09;
        }
    }
}

void main() {
    isr_install();
    clear_screen();
    __asm__ __volatile__("int $2");
}