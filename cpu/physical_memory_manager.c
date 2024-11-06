#include "physical_memory_manager.h"
#include "memory_consts.h"

/* This should be computed at link time, but a hardcoded
 * value is fine for now. Remember that our kernel starts
 * at 0x1000 as defined on the Makefile */
static uint32_t free_mem_addr = 0x10000;
static uint32_t* frames;
static uint64_t n_frames;

void init_physical_memory_manager() {
   uint32_t mem_end_page = 0x8000000;

   n_frames = mem_end_page / 0x1000;
   frames = (uint32_t*)phys_kmalloc(INDEX_FROM_BIT(n_frames), 0, NULL);
   memory_set(frames, 0, INDEX_FROM_BIT(n_frames));

   kprint("Physical memory manager initialized\n");
}

// Static function to set a bit in the frames bitset
static void set_frame(uint32_t frame_addr)
{
   uint32_t frame = frame_addr / 0x1000; 
   uint32_t idx = INDEX_FROM_BIT(frame);
   uint32_t off = OFFSET_FROM_BIT(frame);
   frames[idx] |= (0x1 << off);
}

// Static function to test if a bit is set.
static uint32_t test_frame(uint32_t frame_addr)
{
   uint32_t frame = frame_addr / 0x1000;
   uint32_t idx = INDEX_FROM_BIT(frame);
   uint32_t off = OFFSET_FROM_BIT(frame);
   return (frames[idx] & (0x1 << off));
}

// Static function to find the first free frame.
uint32_t find_first_free_frame()
{
   uint32_t i, j;
   for (i = 0; i < INDEX_FROM_BIT(n_frames); i++)
   {
       // nothing free, exit early
       if (frames[i] == 0xFFFFFFFF) { 
           continue;
       }

       // at least one bit is free here.
       for (j = 0; j < 32; j++)
       {
           uint32_t toTest = 0x1 << j;
           if (!(frames[i]&toTest)) {
                return i*4*8+j;
            }
        }
   }

    return -1;
}

// Public functions

// Function to assign a frame to a page
uint32_t alloc_frame()
{
    uint32_t idx = find_first_free_frame(); 
    if (idx == -1) {
        return -1;
    }
    // idx is now the index of the first free frame.

    set_frame(idx*0x1000); // this frame is now ours!
    return idx;
}

// Static function to clear a bit in the frames bitset
void free_frame(uint32_t frame_addr)
{
   uint32_t frame = frame_addr / PAGE_SIZE;
   uint32_t idx = INDEX_FROM_BIT(frame);
   uint32_t off = OFFSET_FROM_BIT(frame);
   frames[idx] &= ~(0x1 << off);
}

uint32_t phys_kmalloc(size_t size, int align, uint32_t *phys_addr) {
    if (free_mem_addr + size > QEMU_MAX_MEMORY) {
        kprint("PHYSICAL MEMORY ALLOCATION FAILED!\n");
        return 0;
    }

    if (align == 1 && (free_mem_addr & 0x00000FFF)) {
        free_mem_addr &= 0xFFFFF000;
        free_mem_addr += 0x1000;
    }
    if (phys_addr) *phys_addr = free_mem_addr;
    uint32_t ret = free_mem_addr;
    free_mem_addr += size;
    return ret;
}

uint32_t get_used_frames() {
    return n_frames - (free_mem_addr / PAGE_SIZE);
}

uint32_t get_total_frames() {
    return n_frames;
}