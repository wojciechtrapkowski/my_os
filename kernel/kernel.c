#include "../drivers/screen.h"

/* This will force us to create a kernel entry function instead of jumping to kernel.c:0x00 */
void dummy_test_entrypoint() {
}

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
    clear_screen();
    kprint_at("X", 1, 6);
    kprint_at("This text spans multiple lines", 75, 10);
    kprint_at("There is a line\nbreak", 0, 20);
    kprint("There is a line\nbreak");
    kprint_at("What happens when we run out of space?", 45, 24);
}