// @brief Port I/O functions
unsigned char port_byte_in(unsigned short port) {
    unsigned char result;

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

void port_byte_out(unsigned short port, unsigned char data) {
    /*
        * Inline assembly syntax:
        * out %al, %dx
        * Write the byte in al to I/O port specified in dx
        * "a" (data) means the output operand (data) is allocated in the al register
        * "d" (port) means the input operand (port) is allocated in the dx register
    */

    __asm__("out %%al, %%dx" : : "a" (data), "d" (port));
}

unsigned short port_word_in (unsigned short port) {
    unsigned short result;
    __asm__("in %%dx, %%ax" : "=a" (result) : "d" (port));
    return result;
}

void port_word_out (unsigned short port, unsigned short data) {
    __asm__("out %%ax, %%dx" : : "a" (data), "d" (port));
}