#include "paging.h"
#include "memory_consts.h"
#include "../drivers/disk.h"

page_directory_t* current_directory = NULL;
page_table_t* page_swap = NULL;
uint32_t phys;

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
    kprint("Paging enabled\n");
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
    for (uint32_t i = NUMBER_OF_INITIAL_TABLES; i < 1024; i++) {
        dir->page_directory_entries[i] = NULL;
    }

    return dir;
}

page_table_t* create_table(uint32_t address, page_directory_t* dir) {
    uint32_t table_idx = TABLE_INDEX_MASK(address);
    uint32_t tmp;
    page_table_t* table = (page_table_t*) phys_kmalloc(sizeof(page_table_t), 1, &tmp);
    if (table == 0) {
        kprint("Not enough memory for page table!\n");
        return NULL;
    }
    memory_set(table, 0, sizeof(page_table_t));
    dir->page_directory_entries[table_idx] = table;
    dir->page_directory_entries_physical[table_idx] = tmp | PAGE_PRESENT | PAGE_RW | PAGE_USER;
    

    // Create pages for this table
    for (uint32_t i = 0; i < PAGES_PER_TABLE; i++) {
        // Each page is 4KB (12 bits), so shift i by 12 and add to base address
        // TODO: Think about this
        if (create_page(address | (i << 12), dir) == NULL) {
            kprint("Not enough memory for page!\n");
            return NULL;
        }
    }
    return table;
}

page_t* create_page(uint32_t address, page_directory_t* dir) {
    uint32_t table_idx = TABLE_INDEX_MASK(address);
    uint32_t page_idx = PAGE_INDEX_MASK(address);
    if (!dir->page_directory_entries[table_idx]) {
        page_table_t* table = create_table(address, dir);
        if (table == NULL) {
            kprint("Not enough memory for page table!\n");
            return NULL;
        }
    }
    page_t* page = &(dir->page_directory_entries[table_idx]->pages[page_idx]);

    // Frame was already allocated
    if (page->frame != 0) {
       return page;
    }

    uint32_t idx = alloc_frame();
    if (idx == -1) {
        return NULL;
    }
    page->present = 1; // Mark it as present.
    page->rw = 1; // Should the page be writeable?
    page->user = 0; // Should the page be user-mode?
    page->frame = idx;

    return page;
}

page_t* get_page(uint32_t virtual_address, page_directory_t* dir) {
    uint32_t table_idx = TABLE_INDEX_MASK(virtual_address);
    uint32_t page_idx = PAGE_INDEX_MASK(virtual_address);

    // Create page table only when needed - lazy allocation
    if (dir->page_directory_entries[table_idx] == NULL) {
        kprint("Creating page table!\n");
        page_table_t* table = create_table(virtual_address, dir);
        if (table == NULL) {
            kprint("Not enough memory for page table!\n");
            return NULL;
        }
    } else {
        // kprint("Page table already exists! Table ID: \n");
        // char num[16];
        // int_to_ascii(table_idx, num);
        // kprint(num);
        // kprint("\n");
    }

    return &(dir->page_directory_entries[table_idx]->pages[page_idx]);
}


void free_page(uint32_t virtual_address, page_directory_t* dir) {
    page_t* page = get_page(virtual_address, dir);
    if (page->frame == 0) {
        return;
    }

    free_frame(page->frame);
    page->frame = 0x0;
}

static size_t swap_index = 0;
static size_t get_free_swap_index() {
    if (swap_index >= 256) {
        PANIC("No more swap space available!");
    }
    return swap_index++;    
}

static page_table_t* select_page_to_evict() {
    phys = current_directory->page_directory_entries_physical[20];
    return current_directory->page_directory_entries[20];
}

void page_fault(registers_t* regs) {
    // Disable interrupts while handling the page fault.
    asm("cli");

    // Get the faulting address from the CR2 register.
    uint32_t faulting_address;
    asm ("mov %%cr2, %0" : "=r" (faulting_address));

    // Error code analysis
    int not_present = !(regs->err_code & 0x1); // Page not present
    int rw = regs->err_code & 0x2;             // Write operation?
    int us = regs->err_code & 0x4;             // Processor was in user-mode?
    int reserved = regs->err_code & 0x8;       // Overwritten CPU-reserved bits of page entry?
    int id = regs->err_code & 0x10;            // Caused by an instruction fetch?

    // Display error information
    kprint("Page fault! ( ");
    if (not_present) { kprint("not present "); }
    if (rw) { kprint("read-only "); }
    if (us) { kprint("user-mode "); }
    if (reserved) { kprint("reserved "); }
    kprint_hex(faulting_address);
    kprint(" )\n");

    // Locate the page and page table for the faulting address.
    page_t* page = get_page(faulting_address, current_directory);
    page_table_t* page_table = current_directory->page_directory_entries[TABLE_INDEX_MASK(faulting_address)];

    // If the page is already present, there's no need to swap; return immediately.
    if (page != NULL && page->present) {
        asm("sti");
        return;
    }

    kprint("Need to perform swap!\n");

    // If there is no previously selected page to swap out, select one to evict.
    if (page_swap == NULL) {
        page_swap = select_page_to_evict(); 
    }

    // There is no memory to create new page, swap out the selected page
    size_t idx = find_first_free_frame();
    uint32_t swap_index = get_free_swap_index();
    uint32_t page_index_on_disk = 0;

    if (page == NULL || idx == -1) {
        void* frame_addr = (void*)(page_swap->pages[0].frame << 12);
        if (!page->present) {
            page_index_on_disk = page->frame;
        }
        // Write the selected page to disk swap area.
        ata_dma_write(2048 + (swap_index * 8), 0, 8, frame_addr);
        wait_for_disk();

        free_frame(page_swap->pages[0].frame);

        page = create_page(faulting_address, current_directory);
        if (page == NULL) {
            PANIC("Failed to create page!");
        }
        // Update the swap data for the evicted page.
        page_swap->pages[0].present = 0;
        page_swap->pages[0].frame = swap_index;

        kprint("Page swapped out!\n");

        // Mark `page_swap` as NULL, freeing it for the next eviction.
        page_swap = NULL;
    }

    // If the faulting page is not present, perform a swap-in operation.
    if (!page->present) {
        kprint("Page index on disk: ");
        kprint_hex(page_index_on_disk);
        kprint("\n");
        if (idx == -1) {
            idx = page_index_on_disk;
        }
        kprint("Page not present; swapping in!\n");

        // Read the page from swap space into the new frame.
        ata_dma_read(2048 + (page_index_on_disk * 8), 0, 8, (void*)(idx << 12));
        wait_for_disk();

        // Update the page table and mark the page as present.
        page->present = 1;
        page->rw = 1;
        page->user = us;
        page->frame = idx;

        kprint("Page swapped in!\n");

    }

    // Invalidate the old TLB - we could do it only for the faulting address & swapped out page
    // but it's simpler this way.
    switch_page_directory(current_directory);

    // Re-enable interrupts.
    asm("sti");
}

void paging_test_swap() {
    kprint("Trying first write...\n");

    // First access - within valid range
    uint32_t virt_addr = (20 << 22);  // Directory 20, offset 0
    uint8_t* memory = (uint8_t*)virt_addr;
    kprint("First write done\n");
    kprint_hex(*(uint8_t*)memory);
    kprint("\n");
}