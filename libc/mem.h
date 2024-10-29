#ifndef MEM_H
#define MEM_H

#include "../cpu/type.h"

#include <stdint.h>
#include <stddef.h>

void memory_copy(uint8_t *source, uint8_t *dest, int nbytes);
void memory_set(void* dest, int val, uint32_t len);

#endif