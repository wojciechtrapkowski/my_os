#include "../drivers/ports.h"

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
    /* Screen cursor position: ask VGA control register (0x3d4) for bytes
     * 14 = high byte of cursor and 15 = low byte of cursor. */
    port_byte_out(0x3d4, 14); /* Requesting byte 14: high byte of cursor pos */
    /* Data is returned in VGA data register (0x3d5) */
    int position = port_byte_in(0x3d5);
    position = position << 8; /* high byte */

    port_byte_out(0x3d4, 15); /* requesting low byte */
    position += port_byte_in(0x3d5);

    /* VGA 'cells' consist of the character and its control data
     * e.g. 'white on black background', 'red text on white bg', etc */
    int offset_from_vga = position * 2;

    /* Let's write on the current cursor position, we already know how
     * to do that */
    char *vga = (char*)0xb8000;
    vga[offset_from_vga] = 'X'; 
    vga[offset_from_vga+1] = 0x0f; 
}