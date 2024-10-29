#include "mem.h"
#include "function.h"
void memory_copy(uint8_t *source, uint8_t *dest, int nbytes) {
    int i;
    for (i = 0; i < nbytes; i++) {
        *(dest + i) = *(source + i);
    }
}

void memory_set(void* dest, int val, uint32_t len) {
    unsigned char *ptr = (unsigned char*)dest;
    while (len-- > 0)
        *ptr++ = (unsigned char)val;
}