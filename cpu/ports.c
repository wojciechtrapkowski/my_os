#include "ports.h"

// @brief Port I/O functions

u8 port_byte_in (u16 port) {
    u8 result;

    /*
        * Inline assembly syntax:
        * in %dx, %al
        * Read a byte from I/O port specified in dx into al
        * "=a" (result) means output operand (result) is allocated in the al register
        * "d" (port) means input operand (port) is allocated in the dx register
    */

    __asm__("in %%dx, %%al" : "=a" (result) : "d" (port));
    return result;
}

void port_byte_out (u16 port, u8 data) {
    /*
        * Inline assembly syntax:
        * out %al, %dx
        * Write the byte in al to I/O port specified in dx
        * "a" (data) means the output operand (data) is allocated in the al register
        * "d" (port) means the input operand (port) is allocated in the dx register
    */

    __asm__  __volatile__("out %%al, %%dx" : : "a" (data), "d" (port));
}


u16 port_word_in (u16 port) {
    u16 result;
    __asm__("in %%dx, %%ax" : "=a" (result) : "d" (port));
    return result;
}

void port_word_out (u16 port, u16 data) {
    __asm__  __volatile__("out %%ax, %%dx" : : "a" (data), "d" (port));
}