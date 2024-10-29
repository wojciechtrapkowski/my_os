#include <stdint.h>
#include <stddef.h>

#include "paging.h"


uint32_t kmalloc(size_t size, int align, uint32_t *phys_addr);
void alloc_frame(page_t *page, int is_kernel, int is_writeable);
void free_frame(page_t *page);
