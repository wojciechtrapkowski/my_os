#include "paging.h"
#include "physical_memory_manager.h"
#include "../libc/mem.h"
#include "isr.h"
#include "../drivers/screen.h"

page_directory_t* current_directory;

void enable_paging() {
    uint32_t cr0;
    asm volatile("mov %%cr0, %0": "=r"(cr0));
    cr0 |= 0x00010000;     // Set Write Protec bit
    cr0 |= 0x80000000;     // Set Paging bit
    asm volatile("mov %0, %%cr0":: "r"(cr0));
}

void init_paging()
{
    // The size of physical memory. For the moment we
    // assume it is 16MB big.
    uint32_t mem_end_page = 0x1000000;

    n_frames = mem_end_page / 0x1000;
    frames = (uint32_t*)kmalloc(INDEX_FROM_BIT(n_frames), 0, NULL);
    memory_set(frames, 0, INDEX_FROM_BIT(n_frames));

    // Creating kernel page directory
    page_directory_t* kernel_directory = (page_directory_t*)kmalloc(sizeof(page_directory_t), 1, NULL);
    memory_set(kernel_directory, 0, sizeof(page_directory_t));
    current_directory = kernel_directory;

    // Map 16MB (4 page tables worth)
    // We need to identity map (phys addr = virt addr) from
    // 0x0 to the end of used memory, so we can access this
    // transparently, as if paging wasn't enabled.
    // NOTE that we use a while loop here deliberately.
    // inside the loop body we actually change placement_address
    // by calling kmalloc(). A while loop causes this to be
    // computed on-the-fly rather than once at the start.
    for(uint32_t i = 0; i < mem_end_page; i += 0x1000) {
        uint32_t is_writeable = (i <= 0xBFFFF);
        alloc_frame(get_page(i, 1, kernel_directory), /*is_kernel*/ 0, /*writable*/ is_writeable);
    }

    register_interrupt_handler(14, page_fault);

    switch_page_directory(kernel_directory);
    enable_paging();
}

void switch_page_directory(page_directory_t* dir)
{
   current_directory = dir;
   asm volatile("mov %0, %%cr3":: "r"(&dir->tablesPhysical));
}

page_t* get_page(uint32_t address, int make, page_directory_t* dir)
{
   // Turn the address into an index
   address /= 0x1000;
   // Find the page table containing this address
   uint32_t table_idx = address / 1024;
   // If this table is already assigned
   if (dir->tables[table_idx]) {
       return &dir->tables[table_idx]->pages[address%1024];
   }
   else if(make) {
       uint32_t tmp;
       dir->tables[table_idx] = (page_table_t*)kmalloc(sizeof(page_table_t), 1,&tmp);
       memory_set(dir->tables[table_idx], 0, 0x1000);
       dir->tablesPhysical[table_idx] = tmp | 0x7; // PRESENT, RW, US.
       return &dir->tables[table_idx]->pages[address%1024];
   }
   else {
       return 0;
   }
}

void page_fault(registers_t* regs) {
    // A page fault has occurred.
    // The faulting address is stored in the CR2 register.
    uint32_t faulting_address;
    asm volatile("mov %%cr2, %0" : "=r" (faulting_address));

    // The error code gives us details of what happened.
    int present   = !(regs->err_code & 0x1); // Page not present
    int rw = regs->err_code & 0x2;           // Write operation?
    int us = regs->err_code & 0x4;           // Processor was in user-mode?
    int reserved = regs->err_code & 0x8;     // Overwritten CPU-reserved bits of page entry?
    int id = regs->err_code & 0x10;          // Caused by an instruction fetch?

    // Output an error message.
    kprint("Page fault! ( ");
    if (present) {kprint("present ");}
    if (rw) {kprint("read-only ");}
    if (us) {kprint("user-mode ");}
    if (reserved) {kprint("reserved ");}
    kprint_hex(faulting_address);
    kprint(" ) \n");

    // // TEMPORARY!!!
    // // Skip the instruction that caused the fault
    // unsigned char* instruction = (unsigned char*)regs->eip;
    // int skip_bytes = (instruction[0] >= 0x88 && instruction[0] <= 0x8B) ? 2 : 1;
    // regs->eip += skip_bytes;
}
