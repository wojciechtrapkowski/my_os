/**
 * @brief Bitmap heap implementation
 * @see https://wiki.osdev.org/User:Pancakes/BitmapHeapImplementation
 */

#ifndef HEAP_H
#define HEAP_H

#include "../libc/mem.h"

#include <stdint.h>

#define KHEAP_BITMAP_RESERVED_BIT 5

typedef struct KHEAPBLOCK {
    struct KHEAPBLOCK* next;
    uint32_t size;
    uint32_t used;
    uint32_t bsize;
    uint32_t lfb;
} KHEAPBLOCK_T;

typedef struct KHEAP {
    KHEAPBLOCK_T* fblock;
} KHEAP_T;

void init_heap();
void* kmalloc(uint32_t size);
void kfree(void* ptr);

#endif