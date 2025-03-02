#ifndef PAGING_H
#define PAGING_H

#include "isr.h"
#include "physical_memory_manager.h"

#define LOG_PAGING 0

// TODO: Find better place for this
#define PANIC(msg) do { \
    kprint("PANIC: "); \
    kprint(msg); \
    kprint("\n"); \
    asm volatile("cli"); \
    asm volatile("hlt"); \
    for(;;); \
} while(0)

// Address masks
#define TABLE_INDEX_MASK(x) ((x >> 22))           // Get table index
#define PAGE_INDEX_MASK(x) ((x >> 12) & 0x3FF)    // Get index of page in table
#define OFFSET_MASK(x) ((x) & 0xFFF)              // Get offset withing page
#define PAGE_ALIGN (~(PAGE_SIZE - 1))


#define SWAP_SIZE_MB     16                          // 16MB swap space
#define PAGE_SIZE        4096                        // 4KB pages
#define PAGES_IN_SWAP    (SWAP_SIZE_MB * 1024 * 1024) / PAGE_SIZE
#define BITMAP_SIZE      32                           // 4 MB

// Page flags
#define PAGE_PRESENT   0x1 
#define PAGE_RW        0x2 
#define PAGE_USER      0x4 
#define PAGE_ACCESSED  0x20
#define PAGE_DIRTY     0x40

typedef struct page
{
   uint32_t present    : 1;   // Page present in memory
   uint32_t rw         : 1;   // Read-only if clear, readwrite if set
   uint32_t user       : 1;   // Supervisor level only if clear
   uint32_t accessed   : 1;   // Has the page been accessed since last refresh?
   uint32_t dirty      : 1;   // Has the page been written to since last refresh?
   uint32_t unused     : 7;   // Amalgamation of unused and reserved bits
   uint32_t frame      : 20;  // Frame address (shifted right 12 bits)
} page_t;

typedef struct page_table
{
   page_t pages[1024];
} page_table_t;


typedef struct page_directory
{
   page_table_t* page_directory_entries[1024];
   uint32_t page_directory_entries_physical[1024];
   /**
      The physical address of page_directory_entries_physical. This comes into play
      when we get our kernel heap allocated and the directory
      may be in a different location in virtual memory.
   **/
   uint32_t physicalAddr;
} page_directory_t;

typedef struct {
    uint32_t last_run_time;
    uint32_t normal_interval;    
    uint32_t urgent_interval;    
    uint32_t fault_count;        // Page faults since last run
    uint32_t fault_threshold;    // Run if faults exceed this
} crawler_state_t;

typedef struct {
    uint32_t total_pages;
    uint32_t accessed_pages;
    uint32_t dirty_pages;
    uint32_t clean_pages;
} page_stats_t;
    

void init_paging();

void switch_page_directory(page_directory_t* new_dir);

void page_fault(registers_t* regs);

page_directory_t* create_page_directory();

page_table_t* create_table(uint32_t address, page_directory_t* dir);

page_t* create_page(uint32_t address, page_directory_t* dir);

page_t* get_page(uint32_t virtual_address, page_directory_t* dir);

void free_page(uint32_t virtual_address, page_directory_t* dir);

void free_page_table(page_table_t* table);  

void free_page_directory(page_directory_t* dir);

void paging_test_swap();
#endif
