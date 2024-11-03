#include "heap.h"
#include "../drivers/screen.h"
#include <stddef.h>

static KHEAP_T kheap;

/**
 * @brief Add a block to the heap
 * 
 * @details 
 * 1. Set up block header
 * 2. Initialize bitmap to track allocations
 * 3. Reserve space for bitmap itself
 * 4. Add to heap's block list
 * 
 * @param heap Heap structure
 * @param addr Address of the block
 * @param size Size of the block
 * @param block_size Size of each block
 * @return 1 on success, 0 on failure
 */
static int kheap_add_block(uintptr_t addr, uint32_t size, uint32_t block_size) {
    KHEAPBLOCK_T* block = (KHEAPBLOCK_T*)addr;

    block->next = kheap.fblock;
    kheap.fblock = block;
    block->size = size - sizeof(KHEAPBLOCK_T);
    block->bsize = block_size;

    uint32_t block_count = block->size / block->bsize;
    // Get a pointer to the memory right after the block header - this will be our bitmap
    uint8_t* bitmap = (uint8_t*)&block[1];
    
    // Initialize bitmap to zeros
    for (uint32_t i = 0; i < block_count; i++) {
        bitmap[i] = 0;
    }

    // Calculate and reserve space for the bitmap itself
    uint32_t bitmap_blocks = (block_count / block_size) * block_size < block_count 
        ? block_count / block_size + 1 
        : block_count / block_size;
    
     // Mark bitmap space as used
    for (uint32_t i = 0; i < bitmap_blocks; i++) {
        bitmap[i] = KHEAP_BITMAP_RESERVED_BIT;
    }

    // Set last free block index and used count
    block->lfb = bitmap_blocks - 1;
    block->used = bitmap_blocks;
    
    return 1;
}

/**
 * @brief Get the next available block ID in the heap bitmap
 * @param a Current block ID
 * @param b Block ID to avoid (usually already in use)
 * @return Next available block ID
 * 
 * @details This function finds the next available block ID for heap allocation.
 * It increments from the current ID (a) while avoiding:
 * - The specified ID to avoid (b)
 * - Zero (0), which is reserved
 * This ensures we get a unique, non-zero ID for the next block allocation.
 */
static uint8_t kheap_get_next_id(uint8_t a, uint8_t b) {
    uint8_t c;
    for (c = a + 1; c == b || c == 0; ++c);
    return c;
}

/**
 * @brief Allocate memory from the heap
 * 
 * @details 
 * 1. Find block with enough space
 * 2. Search bitmap for consecutive free blocks
 * 3. Mark blocks as allocated with unique ID
 * 4. Return pointer to allocated memory
 * 
 * @param heap Heap structure
 * @param size Size of the allocation
 * @return Pointer to the allocated memory, or NULL if allocation failed
 */
static void* kheap_alloc(uint32_t size) {
    KHEAPBLOCK_T* block;
    uint8_t* bitmap;
    uint32_t block_count;
    uint32_t blocks_needed;
    uint8_t next_id;

    if (size == 0) {
        return NULL;
    }

    for (block = kheap.fblock; block != NULL; block = block->next) {
        // Check if there's enough space in the current block
        if (block->size - (block->used * block->bsize) < size) {
            continue;
        }
        
        block_count = block->size / block->bsize;
        blocks_needed = (size / block->bsize) * block->bsize < size 
            ? size / block->bsize + 1 
            : size / block->bsize;
        
        bitmap = (uint8_t*)&block[1];
        
        // Start searching from last free block
        uint32_t start = (block->lfb + 1 >= block_count) ? 0 : block->lfb + 1;
        
        for (uint32_t i = start; i < block_count; ++i) {
            if (i >= block_count) {
                i = 0;
            }
            
            if (bitmap[i] == 0) {
                // Count consecutive free blocks
                uint32_t free_count = 0;
                while ((i + free_count) < block_count && 
                        bitmap[i + free_count] == 0 && 
                        free_count < blocks_needed) {
                    free_count++;
                }
                
                // If we found enough consecutive blocks
                if (free_count == blocks_needed) {
                    // Get unique ID that doesn't match neighbors
                    next_id = kheap_get_next_id(
                        i > 0 ? bitmap[i - 1] : 0,
                        (i + free_count) < block_count ? bitmap[i + free_count] : 0
                    );
                    
                    // Mark blocks as allocated
                    for (uint32_t j = 0; j < free_count; ++j) {
                        bitmap[i + j] = next_id;
                    }
                    
                    // Update block metadata
                    block->lfb = i;
                    block->used += free_count;
                    
                    void* allocated = (void*)((uintptr_t)&block[1] + (i * block->bsize));
                    
                    return allocated;
                }
                
                // Skip ahead
                i += free_count;
            }
        }
    }

    return NULL;
}

/**
 * @brief Free memory allocated from the heap
 * 
 * @details 
 * 1. Find block containing pointer
 * 2. Calculate bitmap index
 * 3. Free all blocks with same ID
 * 4. Update block metadata
 * 
 * @param heap Heap structure
 * @param ptr Pointer to the allocated memory
 * @return void
 */
static void kheap_free(void* ptr) {
    if (!ptr) return; 

    KHEAPBLOCK_T* block;
    uint8_t* bitmap;
    uint32_t block_index;
    uint8_t block_id;
    
    // Iterate through heap blocks to find the one containing our pointer
    for (block = kheap.fblock; block; block = block->next) {
        uintptr_t block_start = (uintptr_t)block;
        uintptr_t block_end = block_start + sizeof(KHEAPBLOCK_T) + block->size;
        
        // Check if pointer belongs to this block
        if ((uintptr_t)ptr > block_start && (uintptr_t)ptr < block_end) {
            // Calculate offset from block data start
            uintptr_t ptr_offset = (uintptr_t)ptr - (uintptr_t)&block[1];
            
            // Calculate which block in bitmap this pointer belongs to
            block_index = ptr_offset / block->bsize;
            
            // Get bitmap pointer
            bitmap = (uint8_t*)&block[1];
            
            // Get the ID of the allocated block
            block_id = bitmap[block_index];
            
            // Calculate maximum number of blocks
            uint32_t max_blocks = block->size / block->bsize;
            
            // Free all blocks with matching ID
            uint32_t freed_blocks;
            for (freed_blocks = 0; 
                 (block_index + freed_blocks) < max_blocks && 
                 bitmap[block_index + freed_blocks] == block_id; 
                 ++freed_blocks) {
                bitmap[block_index + freed_blocks] = 0;
            }
            
            // Update block metadata
            block->used -= freed_blocks;
            
            // Update last free block if necessary
            if (block_index < block->lfb) {
                block->lfb = block_index;
            }
            
            return;
        }
    }

    return;
}

/**
 * @brief Allocate memory from the heap
 * 
 * In the future, we will add handling for allocating more memory, if needed.
 * 
 * @param size Size of the allocation
 * @return Pointer to the allocated memory
 */
void* kmalloc(uint32_t size) {
    return kheap_alloc(size);
}

/**
 * @brief Free memory allocated from the heap
 * 
 * @param ptr Pointer to the allocated memory
 */
void kfree(void* ptr) {
    kheap_free(ptr);
}

/**
 * @brief Initialize the heap
 * @param heap Heap structure
 */
void init_heap() {
    kheap.fblock = NULL;
    kheap_add_block(0x100000, 0x100000, 16);  /* add block to heap 
                                                (starting 1MB mark and length of 1MB) 
                                                with default block size of 16 bytes */
    kprint("Heap initialized\n");
}