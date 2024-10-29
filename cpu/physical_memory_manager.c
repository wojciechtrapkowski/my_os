#include "physical_memory_manager.h"
#include "../drivers/screen.h"

/* This should be computed at link time, but a hardcoded
 * value is fine for now. Remember that our kernel starts
 * at 0x1000 as defined on the Makefile */
uint32_t free_mem_addr = 0x10000;

uint32_t* frames;
size_t n_frames;

// Static function to set a bit in the frames bitset
static void set_frame(uint32_t frame_addr)
{
   uint32_t frame = frame_addr / 0x1000; // 4 kB
   uint32_t idx = INDEX_FROM_BIT(frame);
   uint32_t off = OFFSET_FROM_BIT(frame);
   frames[idx] |= (0x1 << off);
}

// Static function to clear a bit in the frames bitset
static void clear_frame(uint32_t frame_addr)
{
   uint32_t frame = frame_addr/0x1000;
   uint32_t idx = INDEX_FROM_BIT(frame);
   uint32_t off = OFFSET_FROM_BIT(frame);
   frames[idx] &= ~(0x1 << off);
}

// Static function to test if a bit is set.
static uint32_t test_frame(uint32_t frame_addr)
{
   uint32_t frame = frame_addr/0x1000;
   uint32_t idx = INDEX_FROM_BIT(frame);
   uint32_t off = OFFSET_FROM_BIT(frame);
   return (frames[idx] & (0x1 << off));
}

// Static function to find the first free frame.
static uint32_t find_first_free_frame()
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

   // no free frames
   return (uint32_t)-1;
}

// Public functions

// Function to allocate a frame
void alloc_frame(page_t *page, int is_kernel, int is_writeable)
{
   if (page->frame != 0) {
       // Frame was already allocated, return straight away.
       return; 
   } else {
       uint32_t idx = find_first_free_frame(); 
       
       // idx is now the index of the first free frame.

       if (idx == (uint32_t)-1)
       {
           // PANIC is just a macro that prints a message to the screen then hits an infinite loop.
           PANIC("No free frames!");
       }
       set_frame(idx*0x1000); // this frame is now ours!
       page->present = 1; // Mark it as present.
       page->rw = (is_writeable)?1:0; // Should the page be writeable?
       page->user = (is_kernel)?0:1; // Should the page be user-mode?
       page->frame = idx;
   }
}

// Function to deallocate a frame.
void free_frame(page_t *page)
{
   uint32_t frame;
   if (!(frame=page->frame)) {
       // The given page didn't actually have an allocated frame!
       return; 
   }
   else {
       clear_frame(frame); // Frame is now free again.
       page->frame = 0x0; // Page now doesn't have a frame.
   }
}

uint32_t kmalloc(size_t size, int align, uint32_t *phys_addr) {
    if (align == 1 && (free_mem_addr & 0x00000FFF)) {
        free_mem_addr &= 0xFFFFF000;
        free_mem_addr += 0x1000;
    }
    if (phys_addr) *phys_addr = free_mem_addr;
    uint32_t ret = free_mem_addr;
    free_mem_addr += size;
    return ret;
}


