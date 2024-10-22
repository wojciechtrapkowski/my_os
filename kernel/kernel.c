/* This will force us to create a kernel entry function instead of jumping to kernel.c:0x00 */
void dummy_test_entrypoint() {
}

void main() {
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