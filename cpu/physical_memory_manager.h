#ifndef PHYSICAL_MEMORY_MANAGER_H
#define PHYSICAL_MEMORY_MANAGER_H

#include <stdint.h>
#include <stddef.h>

#include "../drivers/screen.h"
#include "../libc/mem.h"

#define INDEX_FROM_BIT(a) (a/(8*4))
#define OFFSET_FROM_BIT(a) (a%(8*4))
#define PANIC(msg) kprint(msg); while(1);
#define QEMU_MAX_MEMORY 0x8000000


void init_physical_memory_manager();

uint32_t phys_kmalloc(size_t size, int align, uint32_t *phys_addr);

uint32_t find_first_free_frame();

uint32_t alloc_frame();

void free_frame(uint32_t frame_indx);

uint32_t get_used_frames();

uint32_t get_total_frames();

#endif