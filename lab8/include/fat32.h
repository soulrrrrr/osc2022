#ifndef _FAT32_H
#define _FAT32_H

// Ref: https://en.wikipedia.org/wiki/Design_of_the_FAT_file_system#Bootsector
// FAT32 Extended BIOS Parameter Block
#include "typedef.h"
#include "vfs.h"

// block size
#define FAT_BLOCK_SIZE 512
#define FAT_ENTRY_PER_BLOCK (FAT_BLOCK_SIZE / sizeof(int))
#define EOC 0xFFFFFFF

struct fat32_boot_sector {
    char jump[3];  // 0x0
    char oem[8];   // 0x3

    // BIOS Parameter Block (DOS2.0 BPB)
    uint16_t bytes_per_logical_sector;   // 0xB-0xC
    uint8_t logical_sector_per_cluster;  // 0xD
    uint16_t n_reserved_sectors;         // 0xE-0xF
    uint8_t n_file_alloc_tabs;           // 0x10
    uint16_t n_max_root_dir_entries_16;  // 0x11-0x12
    uint16_t n_logical_sectors_16;       // 0x13-0x14
    uint8_t media_descriptor;            // 0x15
    uint16_t logical_sector_per_fat_16;  // 0x16-0x17

    // DOS3.31 BPB
    uint16_t physical_sector_per_track;  // 0x18-0x19
    uint16_t n_heads;                    // 0x1A-0x1B
    uint32_t n_hidden_sectors;           // 0x1C-0x1F
    uint32_t n_sectors_32;               // 0x20-0x23

    // FAT32 Extended BIOS Parameter Block
    uint32_t n_sectors_per_fat_32;              // 0x24-0x27
    uint16_t mirror_flag;                       // 0x28-0x29
    uint16_t version;                           // 0x2A-0x2B
    uint32_t root_dir_start_cluster_num;        // 0x2C-0x2F
    uint16_t fs_info_sector_num;                // 0x30-0x31
    uint16_t boot_sector_bak_first_sector_num;  // 0x32-0x33
    uint32_t reserved[3];                       // 0x34-0x3F
    uint8_t physical_drive_num;                 // 0x40
    uint8_t unused;                             // 0x41
    uint8_t extended_boot_signature;            // 0x42
    uint32_t volume_id;                         // 0x43-0x46
    uint8_t volume_label[11];                   // 0x47-0x51
    uint8_t fat_system_type[8];                 // 0x52-0x59
} __attribute__((packed));

struct fat32_metadata {
    uint32_t fat_region_blk_idx;
    uint32_t n_fat;
    uint32_t sector_per_fat;
    uint32_t data_region_blk_idx;
    uint32_t first_cluster;
    uint8_t sector_per_cluster;
};

struct fat32_dirent {
    uint8_t name[8];            // 0x0-0x7
    uint8_t ext[3];             // 0x8-0xA
    uint8_t attr;               // 0xB
    uint8_t reserved;           // 0xC
    uint8_t create_time[3];     // 0xD-0xF
    uint16_t create_date;       // 0x10-0x11
    uint16_t last_access_date;  // 0x12-0x13
    uint16_t cluster_high;      // 0x14-0x15
    uint32_t ext_attr;          // 0x16-0x19
    uint16_t cluster_low;       // 0x1A-0x1B
    uint32_t size;              // 0x1C-0x1F
} __attribute__((packed));

struct fat32_internal {
    int type;
    char *name;
    struct vnode *vnode;
    struct fat32_internal *parent;
    struct fat32_internal *child[MAX_ENTRIES];
    uint32_t first_cluster;
    uint32_t dirent_cluster;
    uint32_t size;
};

extern struct fat32_metadata fat32_metadata;

int fat32_register();
int fat32_setup_mount(struct filesystem* fs, struct mount* mount);
struct vnode* fat32_create_vnode(struct fat32_internal* fat32_node);
// vnode operations
int fat32_lookup(struct vnode* dir, struct vnode** target, const char* component_name);
int fat32_create(struct vnode* dir, struct vnode** target, const char* component_name);
// int fat32_ls(struct vnode* dir);
// int fat32_mkdir(struct vnode* dir, struct vnode** target, const char* component_name);
// int fat32_load_dentry(struct dentry* dir, char* component_name);

// file operations
int fat32_read(struct file* file, void* buf, uint64_t len);
int fat32_write(struct file* file, const void* buf, uint64_t len);

#endif