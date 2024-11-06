#include "fs.h"

#include "heap.h"
#include "../drivers/disk.h"
#include "../drivers/screen.h"
#include "../libc/string.h"

#define NODE_COUNT 32

static fs_node_t* root;
static fs_node_t nodes[NODE_COUNT];
static int index = 0;

/**
 * @brief Serialize the tree to disk
 */
static void serialize_tree() {
#if FILE_SYSTEM_LOGGING
    kprint("Starting serialization...\n");
#endif
    
    uint32_t sectors_needed = (NODE_COUNT * sizeof(fs_node_t) + 511) / 512;
    fs_node_t* disk_data = (fs_node_t*)kmalloc(sectors_needed * 512);
    memory_set(disk_data, 0, sectors_needed * 512);
    
    // Simple copy is now possible since we use indices instead of pointers
    memory_copy((uint8_t*)nodes, (uint8_t*)disk_data, NODE_COUNT * sizeof(fs_node_t));

#if FILE_SYSTEM_LOGGING
    kprint("Writing to disk...\n");
#endif
    ata_dma_write(FS_TREE_LBA, 0, sectors_needed, disk_data);
    wait_for_disk();
    
    kfree(disk_data);
#if FILE_SYSTEM_LOGGING
    kprint("Serialization complete\n");
#endif
}

/**
 * @brief Deserialize the tree from disk
 */
static void deserialize_tree() {
#if FILE_SYSTEM_LOGGING
    kprint("Starting deserialization...\n");
#endif
    
    uint32_t sectors_needed = (NODE_COUNT * sizeof(fs_node_t) + 511) / 512;
    fs_node_t* disk_data = (fs_node_t*)kmalloc(sectors_needed * 512);
    
    // Read from disk
    ata_dma_read(FS_TREE_LBA, 0, sectors_needed, disk_data);
    wait_for_disk();
    
    // Simple copy is now possible since we use indices
    memory_copy((uint8_t*)disk_data, (uint8_t*)nodes, NODE_COUNT * sizeof(fs_node_t));
    
    // Find root (node with parent_idx == -1 and directory flag)
    root = NULL;
    for (int i = 0; i < NODE_COUNT; i++) {
        if (nodes[i].flags == FS_DIRECTORY && nodes[i].parent_idx == -1) {
            root = &nodes[i];
            break;
        }
    }
    
    if (root == NULL) {
        kprint("Warning: No root node found in deserialized data\n");
    } else {
        kprint("Root node found: ");
        kprint(root->name);
        kprint("\n");
    }
    
    kfree(disk_data);
#if FILE_SYSTEM_LOGGING
    kprint("Deserialization complete\n");
#endif
}

// Initialize filesystem
void init_filesystem() {
    // Clear nodes array
    memory_set(nodes, 0, NODE_COUNT * sizeof(fs_node_t));
    
    // Try to deserialize existing filesystem
    deserialize_tree();
    
    // If no root found, create new filesystem
    if (root == NULL) {
        kprint("Creating new filesystem\n");
        
        // Initialize root node
        root = &nodes[0];
        strcpy(root->name, "/");
        root->parent_idx = -1; 
        root->num_children = 0;
        root->flags = FS_DIRECTORY;
        root->size = 0;
        root->data = NULL;
    }
}

// Helper function to find next free node index
static int find_free_node() {
    for (int i = 0; i < NODE_COUNT; i++) {
        if (nodes[i].flags == 0) {
            return i;
        }
    }
    return -1;
}

void fs_create_directory(fs_node_t* parent, const char* name) {
    if (parent == NULL) {
        parent = root;
    }

    // Check if parent is actually a directory
    if (parent->flags != FS_DIRECTORY) {
        kprint("Error: Parent is not a directory\n");
        return;
    }

    // Check if parent has room for more children
    if (parent->num_children >= MAX_CHILDREN) {
        kprint("Error: Directory is full\n");
        return;
    }

    // Find a free node
    int new_idx = find_free_node();
    if (new_idx == -1) {
        kprint("Error: No free nodes available\n");
        return;
    }

    // Get pointer to new node
    fs_node_t* new_node = &nodes[new_idx];
    
    // Initialize the new directory
    memory_set(new_node, 0, sizeof(fs_node_t)); 
    strcpy(new_node->name, name);
    new_node->flags = FS_DIRECTORY;
    new_node->num_children = 0;
    new_node->size = 0;
    new_node->data = NULL;
    
    // Set parent index
    new_node->parent_idx = ((uint32_t)parent - (uint32_t)nodes) / sizeof(fs_node_t);
    
    // Initialize children indices to -1
    for (int i = 0; i < MAX_CHILDREN; i++) {
        new_node->children_idx[i] = -1;
    }
    
    // Add to parent's children
    parent->children_idx[parent->num_children] = new_idx;
    parent->num_children++;

    // Save changes to disk
    fs_save();
}

void fs_create_file(fs_node_t* parent, const char* name) {
    if (parent == NULL) {
        parent = root;
    }

    // Check if parent is actually a directory
    if (parent->flags != FS_DIRECTORY) {
        kprint("Error: Parent is not a directory\n");
        return;
    }

    // Check if parent has room for more children
    if (parent->num_children >= MAX_CHILDREN) {
        kprint("Error: Directory is full\n");
        return;
    }

    // Find a free node
    int new_idx = find_free_node();
    if (new_idx == -1) {
        kprint("Error: No free nodes available\n");
        return;
    }

    // Get pointer to new node
    fs_node_t* new_node = &nodes[new_idx];
    
    // Initialize the new file
    memory_set(new_node, 0, sizeof(fs_node_t));  // Clear the node first
    strcpy(new_node->name, name);
    new_node->flags = FS_FILE;
    new_node->num_children = 0;
    new_node->size = 0;
    new_node->data = NULL;
    
    // Set parent index
    new_node->parent_idx = ((uint32_t)parent - (uint32_t)nodes) / sizeof(fs_node_t);
    
    // Initialize children indices to -1 (though files won't use them)
    for (int i = 0; i < MAX_CHILDREN; i++) {
        new_node->children_idx[i] = -1;
    }
    
    // Add to parent's children
    parent->children_idx[parent->num_children] = new_idx;
    parent->num_children++;

    // Save changes to disk
    fs_save();
}

/**
 * @brief List the contents of a directory
 * @param node Pointer to the directory node
 */
void fs_list_directory(fs_node_t* node) {
    if (node == NULL) {
        node = root;
    }

    if (node->flags != FS_DIRECTORY) {
        kprint("Not a directory!\n");
        return;
    }
    
    kprint("Listing directory: ");
    kprint(node->name);
    kprint("\n");
    kprint("Number of children: ");
    char num[32];
    int_to_ascii(node->num_children, num);
    kprint(num);
    kprint("\n");

    // Iterate through children indices
    for (uint32_t i = 0; i < node->num_children; i++) {
        int child_idx = node->children_idx[i];
        if (child_idx >= 0 && child_idx < NODE_COUNT) {  // Valid index check
            fs_node_t* child = &nodes[child_idx];
            kprint("- ");
            kprint(child->name);
            kprint(" (");
            if (child->flags == FS_DIRECTORY) {
                kprint("DIR");
            } else if (child->flags == FS_FILE) {
                kprint("FILE");
            }
            kprint(")\n");
        }
    }
}

/**
 * @brief Save the filesystem to disk
 */
void fs_save() {
    serialize_tree();
}
