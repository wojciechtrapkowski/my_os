#ifndef FS_H
#define FS_H

#include <stdint.h>

#define FS_TREE_ADDRESS 0xF00000
#define FS_FILES_ADDRESS 0xFF0000

#define FS_TREE_LBA 36864
#define FS_FILES_LBA 69632

#define FS_FILE 0x01
#define FS_DIRECTORY 0x02

#define MAX_NAME_LENGTH 32
#define MAX_CHILDREN 16

#define FILE_SYSTEM_LOGGING 0

typedef struct fs_node {
    char name[MAX_NAME_LENGTH];

    int parent_idx;
    int children_idx[MAX_CHILDREN];
    uint32_t num_children;

    uint32_t flags;
    uint32_t size;
    uint32_t* data;
} fs_node_t;

void init_filesystem();

void fs_create_directory(fs_node_t* parent, const char* name);
void fs_create_file(fs_node_t* parent, const char* name);

void fs_read_file(fs_node_t* node);
void fs_write_file(fs_node_t* node);

void fs_list_directory(fs_node_t* node);

void fs_save();

#endif
