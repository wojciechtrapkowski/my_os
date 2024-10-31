#ifndef PHYSICAL_MEMORY_MANAGER_H
#define PHYSICAL_MEMORY_MANAGER_H

#include <stdint.h>
#include <stddef.h>

#include "../drivers/screen.h"
#include "../libc/mem.h"

#define INDEX_FROM_BIT(a) (a/(8*4))
#define OFFSET_FROM_BIT(a) (a%(8*4))
#define PANIC(msg) kprint(msg);

void init_physical_memory_manager();

uint32_t kmalloc(size_t size, int align, uint32_t *phys_addr);

uint32_t alloc_frame();

void free_frame(uint32_t frame_indx);

#endif