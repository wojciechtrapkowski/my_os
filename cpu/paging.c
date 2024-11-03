#include "paging.h"
#include "memory_consts.h"

page_directory_t* current_directory = NULL;

void enable_paging() {
    uint32_t cr0;
    asm volatile("mov %%cr0, %0": "=r"(cr0));
    cr0 |= 0x00010000;     // Set Write Protect bit
    cr0 |= 0x80000000;     // Set Paging bit
    asm volatile("mov %0, %%cr0":: "r"(cr0));
}

void init_paging() {
    init_physical_memory_manager();

    page_directory_t* kernel_directory = create_page_directory();

    register_interrupt_handler(14, page_fault);

    switch_page_directory(kernel_directory);

    enable_paging();
}

void switch_page_directory(page_directory_t* dir)
{
   current_directory = dir;
   asm volatile("mov %0, %%cr3":: "r"(dir->page_directory_entries_physical));
}

page_directory_t* create_page_directory() {
    // Allocate directory
    page_directory_t* dir = (page_directory_t*) phys_kmalloc(sizeof(page_directory_t), 1, NULL);
    memory_set(dir, 0, sizeof(page_directory_t));

    // Create few tables
    for (uint32_t i = 0; i < NUMBER_OF_INITIAL_TABLES; i++) {
        // Each table covers 4MB (22 bits)
        create_table(i << 22, dir);
    }

    return dir;
}

void create_table(uint32_t address, page_directory_t* dir) {
    uint32_t table_idx = TABLE_INDEX_MASK(address);
    uint32_t tmp;
    page_table_t* table = (page_table_t*) phys_kmalloc(sizeof(page_table_t), 1, &tmp);
    memory_set(table, 0, sizeof(page_table_t));
    dir->page_directory_entries[table_idx] = table;
    dir->page_directory_entries_physical[table_idx] = tmp | PAGE_PRESENT | PAGE_RW | PAGE_USER;
    

    // Create pages for this table
    for (uint32_t i = 0; i < PAGES_PER_TABLE; i++) {
        // Each page is 4KB (12 bits), so shift i by 12 and add to base address
        create_page(address | (i << 12), dir);
    }
}

page_t* create_page(uint32_t address, page_directory_t* dir) {
    uint32_t table_idx = TABLE_INDEX_MASK(address);
    uint32_t page_idx = PAGE_INDEX_MASK(address);
    if (!dir->page_directory_entries[table_idx]) {
        create_table(address, dir);
    }
    page_t* page = &(dir->page_directory_entries[table_idx]->pages[page_idx]);

    // Frame was already allocated
    if (page->frame != 0) {
       return page; 
    }

    uint32_t idx = alloc_frame();
    page->present = 1; // Mark it as present.
    page->rw = 1; // Should the page be writeable?
    page->user = 0; // Should the page be user-mode?
    page->frame = idx;

    return page;
}

page_t* get_page(uint32_t address, page_directory_t* dir) {
    uint32_t table_idx = TABLE_INDEX_MASK(address);
    uint32_t page_idx = PAGE_INDEX_MASK(address);

    // Create page table only when needed - lazy allocation
    if (!dir->page_directory_entries[table_idx]) {
        create_table(address, dir);
    }

    return &(dir->page_directory_entries[table_idx]->pages[page_idx]);
}


void free_page(uint32_t address, page_directory_t* dir) {
    page_t* page = get_page(address, dir);
    if (page->frame == 0) {
        return;
    }

    free_frame(page->frame);
    page->frame = 0x0;
}

void page_fault(registers_t* regs) {
    asm("cli");
    // A page fault has occurred.
    // The faulting address is stored in the CR2 register.
    uint32_t faulting_address;
    asm ("mov %%cr2, %0" : "=r" (faulting_address));

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

    // Try to handle page fault
    get_page(faulting_address, current_directory);
    asm("sti");
}