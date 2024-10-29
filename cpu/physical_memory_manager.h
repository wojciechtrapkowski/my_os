#include <stdint.h>
#include <stddef.h>

#include "paging.h"

#define INDEX_FROM_BIT(a) (a/(8*4))
#define OFFSET_FROM_BIT(a) (a%(8*4))
#define PANIC(msg) kprint(msg);

extern uint32_t free_mem_addr;
extern uint32_t* frames;
extern size_t n_frames;

uint32_t kmalloc(size_t size, int align, uint32_t *phys_addr);
void alloc_frame(page_t *page, int is_kernel, int is_writeable);
void free_frame(page_t *page);
