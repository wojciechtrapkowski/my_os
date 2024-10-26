#include "../drivers/screen.h"
#include "util.h"

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

    /* Fill up the screen */
    int i = 0;
    for (i = 0; i < 24; i++) {
        char str[255];
        int_to_ascii(i, str);
        kprint_at(str, 0, i);
    }

    kprint_at("This text forces the kernel to scroll. Row 0 will disappear. ", 60, 24);
    kprint("And with this text, the kernel will scroll again, and row 1 will disappear too!");
}