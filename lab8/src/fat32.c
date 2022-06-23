#include "fat32.h"
#include "utils.h"
#include "typedef.h"
#include "memory.h"
#include "printf.h"
#include "sdhost.h"

struct fat32_metadata fat32_metadata;
struct vnode_operations* fat32_v_ops = NULL;
struct file_operations* fat32_f_ops = NULL;

uint32_t get_cluster_blk_idx(uint32_t cluster_idx) {
    return fat32_metadata.data_region_blk_idx +
           (cluster_idx - fat32_metadata.first_cluster) * fat32_metadata.sector_per_cluster;
}

uint32_t get_fat_blk_idx(uint32_t cluster_idx) {
    return fat32_metadata.fat_region_blk_idx + (cluster_idx / FAT_ENTRY_PER_BLOCK);
}

int fat32_register() {
    if (fat32_v_ops != NULL && fat32_f_ops != NULL) {
        return -1;
    }
    fat32_v_ops = (struct vnode_operations*)malloc(sizeof(struct vnode_operations));
    //fat32_v_ops->create = fat32_create;
    fat32_v_ops->lookup = fat32_lookup;
    fat32_v_ops->create = fat32_create;
    //fat32_v_ops->ls = fat32_ls;
    //fat32_v_ops->mkdir = fat32_mkdir;
    fat32_f_ops = (struct file_operations*)malloc(sizeof(struct file_operations));
    fat32_f_ops->read = fat32_read;
    fat32_f_ops->write = fat32_write;
    return 0;
}

int fat32_setup_mount(struct filesystem* fs, struct mount* mount) {
    struct fat32_internal* fat32_root = (struct fat32_internal*)malloc(sizeof(struct fat32_internal));
    fat32_root->type = DIRECTORY;
    fat32_root->name = (char *)malloc(2);
    strcpy(fat32_root->name, "/");
    struct vnode *root_vnode = fat32_create_vnode(fat32_root);
    mount->root = root_vnode;
    mount->fs = fs;
    fat32_root->vnode = root_vnode;
    return 0;
}

struct vnode* fat32_create_vnode(struct fat32_internal* fat32_node) {
    struct vnode* vnode = (struct vnode*)malloc(sizeof(struct vnode));
    vnode->f_ops = fat32_f_ops;
    vnode->v_ops = fat32_v_ops;
    vnode->internal = fat32_node;
    return vnode;
}

int fat32_lookup(struct vnode* dir, struct vnode** target, const char* component_name) {
    // component_name is empty, return dir vnode
    *target = dir;
    printf("[fat32 lookup] %s %x %x %x\n", component_name, dir, dir->mount, dir->mount_parent);
    if (!strcmp(component_name, "")) {
        *target = dir;
        return 0;
    }
    // search component_name in dir
    if (dir->mount != NULL) {
        dir = dir->mount->root;
    }
    printf("target: %s\n", component_name);
    // TODO: second search
    char buf[FAT_BLOCK_SIZE];
    struct fat32_internal* dir_internal = (struct fat32_internal*)dir->internal;
    printf("internal %x\n", dir_internal);
    //if (dir_internal->type != DIRECTORY) return 0;
    uint32_t dirent_cluster = get_cluster_blk_idx(dir_internal->first_cluster);
    readblock(dirent_cluster, buf);
    printf("buf %d\n", dirent_cluster);
    // parse
    struct fat32_dirent* sector_dirent = (struct fat32_dirent*)buf;
    int idx = 0;
    // load all children under dentry
    int found = -1;
    for (int i = 0; sector_dirent[i].name[0] != '\0'; i++) {
        // special value
        if (sector_dirent[i].name[0] == 0xE5) {
            continue;
        }
        // get filename
        char filename[13];
        int len = 0;
        for (int j = 0; j < 8; j++) {
            char c = sector_dirent[i].name[j];
            if (c == ' ') {
                break;
            }
            filename[len++] = c;
        }
        filename[len++] = '.';
        for (int j = 0; j < 3; j++) {
            char c = sector_dirent[i].ext[j];
            if (c == ' ') {
                break;
            }
            filename[len++] = c;
        }
        filename[len++] = 0;
        printf("%s\n", filename);
        if (!strcmp(filename, component_name)) {
            printf("found %s\n", component_name);
            found = 0;
            // create fat32 internal
            struct fat32_internal* child_internal = (struct fat32_internal*)malloc(sizeof(struct fat32_internal));
            child_internal->first_cluster = ((sector_dirent[i].cluster_high) << 16) | (sector_dirent[i].cluster_low);
            child_internal->dirent_cluster = dirent_cluster;
            child_internal->size = sector_dirent[i].size;
            if (sector_dirent[i].attr == 0x10) {  // directory
                child_internal->type = DIRECTORY;
            }
            else {  // file
                child_internal->type = REGULAR_FILE;
            }
            //child_internal->name = malloc(strlen(component_name)+1);
            //strcpy(child_internal->name, component_name);
            // create vnode
            struct vnode* file;
            file = fat32_create_vnode(child_internal);
            child_internal->vnode = file;
            *target = file;
        }
    }
    return found;
}

int fat32_create(struct vnode* dir, struct vnode** target, const char* component_name) {
    printf("[Create]\n");
    char buf[FAT_BLOCK_SIZE];
    struct fat32_internal* dir_internal = (struct fat32_internal*)dir->internal;
    printf("internal %x\n", dir_internal);
    uint32_t dirent_cluster = get_cluster_blk_idx(dir_internal->first_cluster);
    readblock(dirent_cluster, buf);
    printf("buf %d\n", dirent_cluster);
    struct fat32_dirent* sector_dirent = (struct fat32_dirent*)buf;
    int idx = 5;
    // while (sector_dirent[idx].name[0] != '\0') idx++;
    //printf("insert %d\n",  idx);
    //memzero(sector_dirent+idx, 8);
    int t = 0, e = 0;
    while (component_name[t] != '.') {
        sector_dirent[idx].name[t] = component_name[t];
        t++;
    }
    e = t+1;
    while (t < 8) {
        sector_dirent[idx].name[t] = 0x20;
        t++;
    }
    while (component_name[e] != '\0') {
        sector_dirent[idx].ext[t-8] = component_name[e];
        e++;
        t++;
    }
    sector_dirent[idx].attr = 0x20;
    sector_dirent[idx].cluster_low = sector_dirent[idx-1].cluster_low + (uint16_t)((sector_dirent[idx-1].size + FAT_BLOCK_SIZE -1) / FAT_BLOCK_SIZE);
    printf("%d\n", sector_dirent[idx].cluster_low);
    writeblock(dirent_cluster, sector_dirent);
    return 0;
}

// file operations
int fat32_read(struct file* file, void* ret, uint64_t len) {
    struct fat32_internal* file_node = (struct fat32_internal*)file->vnode->internal;
    uint64_t f_pos_ori = file->f_pos;
    uint32_t current_cluster = file_node->first_cluster;
    printf("Current cluster: %u\n", current_cluster);
    int remain_len = len;
    int fat[FAT_ENTRY_PER_BLOCK];
    char buf[512];
    while (remain_len > 0 && current_cluster >= fat32_metadata.first_cluster && current_cluster != EOC) {
        printf("%d\n", get_cluster_blk_idx(current_cluster));
        readblock(get_cluster_blk_idx(current_cluster), ((char *)buf)+file->f_pos);
        for (int i = 0; i < 512; i++) {
            if (buf[i] == '\0' || remain_len-- < 0) break;
            ((char *)ret)[file->f_pos++] = buf[i];
        }
        //file->f_pos += (remain_len < FAT_BLOCK_SIZE) ? remain_len : FAT_BLOCK_SIZE;
        //remain_len -= FAT_BLOCK_SIZE;

        // update cluster number from FAT
        if (remain_len > 0) {
            readblock(get_fat_blk_idx(current_cluster), fat);
            current_cluster = fat[current_cluster % FAT_ENTRY_PER_BLOCK];
        }
    }
    //return 10;
    return (file->f_pos - f_pos_ori);
}

int fat32_write(struct file* file, const void* buf, uint64_t len) {
    struct fat32_internal* file_node = (struct fat32_internal*)file->vnode->internal;
    uint64_t f_pos_ori = file->f_pos;
    int fat[FAT_ENTRY_PER_BLOCK];
    char write_buf[FAT_BLOCK_SIZE];

    // traversal to target cluster using f_pos
    uint32_t current_cluster = file_node->first_cluster;
    int remain_offset = file->f_pos;
    while (remain_offset > 0 && current_cluster >= fat32_metadata.first_cluster && current_cluster != EOC) {
        remain_offset -= FAT_BLOCK_SIZE;
        if (remain_offset > 0) {
            readblock(get_fat_blk_idx(current_cluster), fat);
            current_cluster = fat[current_cluster % FAT_ENTRY_PER_BLOCK];
        }
    }

    // write first block, handle f_pos
    int buf_idx, f_pos_offset = file->f_pos % FAT_BLOCK_SIZE;
    
    printf("write %d %d %d\n", buf_idx, f_pos_offset, get_cluster_blk_idx(current_cluster));
    readblock(get_cluster_blk_idx(current_cluster), write_buf);
    for (buf_idx = 0; buf_idx < FAT_BLOCK_SIZE - f_pos_offset && buf_idx < len; buf_idx++) {
        write_buf[buf_idx + f_pos_offset] = ((char*)buf)[buf_idx];
    }
    printf("w%d\n", get_cluster_blk_idx(current_cluster));
    writeblock(get_cluster_blk_idx(current_cluster), write_buf);
    file->f_pos += buf_idx;

    // write complete block
    int remain_len = len - buf_idx;
    while (remain_len > 0 && current_cluster >= fat32_metadata.first_cluster && current_cluster != EOC) {
        // write block
        printf("w%d\n", get_cluster_blk_idx(current_cluster));
        writeblock(get_cluster_blk_idx(current_cluster), buf + buf_idx);
        file->f_pos += (remain_len < FAT_BLOCK_SIZE) ? remain_len : FAT_BLOCK_SIZE;
        remain_len -= FAT_BLOCK_SIZE;
        buf_idx += FAT_BLOCK_SIZE;

        // update cluster number from FAT
        if (remain_len > 0) {
            readblock(get_fat_blk_idx(current_cluster), fat);
            current_cluster = fat[current_cluster % FAT_ENTRY_PER_BLOCK];
        }
    }

    // TODO: last block also need to handle remainder

    // update file size
    printf("update, %d %d %d\n", file->f_pos, file_node->size, file_node->dirent_cluster);
    if (file->f_pos > file_node->size) {
        file_node->size = file->f_pos;

        // update directory entry
        uint8_t sector[FAT_BLOCK_SIZE];
        readblock(file_node->dirent_cluster, sector);
        struct fat32_dirent* sector_dirent = (struct fat32_dirent*)sector;
        for (int i = 0; sector_dirent[i].name[0] != '\0'; i++) {
            // special value
            if (sector_dirent[i].name[0] == 0xE5) {
                continue;
            }
            // find target file directory entry
            if ((((sector_dirent[i].cluster_high) << 16) | (sector_dirent[i].cluster_low)) == file_node->first_cluster) {
                sector_dirent[i].size = (uint32_t)file->f_pos;
            }
        }
        printf("w%d\n", file_node->dirent_cluster);
        writeblock(file_node->dirent_cluster, sector);
    }

    return file->f_pos - f_pos_ori;
}