#include "ports.h"

// @brief Port I/O functions

uint8_t port_byte_in (uint16_t port) {
    uint8_t result;

    /*
        * Inline assembly syntax:
        * in %dx, %al
        * Read a byte from I/O port specified in dx into al
        * "=a" (result) means output operand (result) is allocated in the al register
        * "d" (port) means input operand (port) is allocated in the dx register
    */

    asm("in %%dx, %%al" : "=a" (result) : "d" (port));
    return result;
}

void port_byte_out (uint16_t port, uint8_t data) {
    /*
        * Inline assembly syntax:
        * out %al, %dx
        * Write the byte in al to I/O port specified in dx
        * "a" (data) means the output operand (data) is allocated in the al register
        * "d" (port) means the input operand (port) is allocated in the dx register
    */

    asm  volatile("out %%al, %%dx" : : "a" (data), "d" (port));
}


uint16_t port_word_in (uint16_t port) {
    uint16_t result;
    asm("in %%dx, %%ax" : "=a" (result) : "d" (port));
    return result;
}

void port_word_out (uint16_t port, uint16_t data) {
    asm  volatile("out %%ax, %%dx" : : "a" (data), "d" (port));
}