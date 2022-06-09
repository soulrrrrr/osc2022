#include "memory.h"
#include "vfs.h"
#include "initramfs.h"
#include "printf.h"
#include "utils.h"
#include "exception.h"

struct vnode_operations* initramfs_v_ops;
struct file_operations* initramfs_f_ops;

int initramfs_register() {
    initramfs_v_ops = (struct vnode_operations*)malloc(sizeof(struct vnode_operations));
    initramfs_v_ops->lookup = &initramfs_lookup;
    initramfs_v_ops->create = &initramfs_create;
    initramfs_v_ops->mkdir = &initramfs_mkdir;
    initramfs_f_ops = (struct file_operations*)malloc(sizeof(struct file_operations));
    initramfs_f_ops->write = &initramfs_write;
    initramfs_f_ops->read = &initramfs_read;
    return 0;
}

int initramfs_setup_mount(struct filesystem* fs, struct mount* mount) {
    struct initramfs_internal* initramfs_root = (struct initramfs_internal*)malloc(sizeof(struct initramfs_internal));
    initramfs_root->type = DIRECTORY;
    initramfs_root->name = (char *)malloc(2);
    strcpy(initramfs_root->name, "/");
    struct node *root_vnode = initramfs_create_vnode(initramfs_root);
    mount->root = root_vnode;
    mount->fs = fs;
    initramfs_root->vnode = root_vnode;
    return 0;
}

struct vnode* initramfs_create_vnode(struct initramfs_internal* initramfs_node) {
    struct vnode* vnode = (struct vnode*)malloc(sizeof(struct vnode));
    vnode->f_ops = initramfs_f_ops;
    vnode->v_ops = initramfs_v_ops;
    vnode->internal = initramfs_node;
    return vnode;
}

int initramfs_create(struct vnode* dir, struct vnode** target, const char* component_name) {
    return -1;
}

// vnode operations
int initramfs_lookup(struct vnode* dir, struct vnode** target, const char* component_name) {
    // component_name is empty, return dir vnode
    if (!strcmp(component_name, "")) {
        return -1;
    }
    else if (!strcmp(component_name, ".")) {
        *target = dir;
        return 0;
    }
    else if (!strcmp(component_name, "..")) { // todo: cross filesystem
        *target = dir;
        if (dir->mount_parent) {
            *target = dir->mount_parent;
            return 0;
        }
        return -1;
    }
    // search component_name in dir
    if (dir->mount != NULL)
        dir = dir->mount->root;
    printf("[INITRAMFS LOOKUP]%s\n", component_name);
    char *program_pos;
    cpio_newc_header *fs = (cpio_newc_header *)0x8000000;
    char *current = (char *)0x8000000;
    int name_size;
    int file_size;
    char *name_pos;
    while (1) {
        fs = (cpio_newc_header *)current;
        name_size = hex_to_int(fs->c_namesize, 8);
        file_size = hex_to_int(fs->c_filesize, 8);
        current += 110;
        if (strcmp(current, "TRAILER!!!") == 0) {
            uart_puts("No such file!\n");
            break;
        }
        name_pos = current;
        if (strcmp(current, component_name) == 0) {
            current += name_size;
            while ((current - (char *)fs) % 4 != 0)
                current++;
            program_pos = (char *)current;
            break;
        } else {
            current += name_size;
            while ((current - (char *)fs) % 4 != 0)
                current++;
            current += file_size;
            while ((current - (char *)fs) % 4 != 0)
                current++;
        }
    }
    char *new_program_pos = (char *)malloc(file_size);
    for (int i = 0; i < file_size; i++) {
        *(new_program_pos+i) = *(program_pos+i);
    }
    printf("name pos : %x\n", name_pos);
    printf("program pos : %x\n", new_program_pos);
    struct initramfs_internal* file_node = (struct initramfs_internal*)malloc(sizeof(struct initramfs_internal));
    file_node->type = REGULAR_FILE;
    file_node->name = name_pos;
    file_node->size = file_size;
    file_node->data = (void *)new_program_pos;
    file_node->vnode = initramfs_create_vnode(file_node);
    *target = file_node->vnode;
    // for (int i = 0; i < MAX_ENTRIES; i++) {
    //     struct initramfs_internal* file_node = ((struct initramfs_internal*)dir->internal)->child[i];
    //     if ((file_node != NULL) & !strcmp(file_node->name, component_name)) {
    //         *target = file_node->vnode;
    //         printf("[lookup] 0x%x\n", *target);
    //         return 0;
    //     }
    // }
    // *target = NULL;
    return 0;
}

int initramfs_write(struct file* file, const void* buf, size_t len) {
    return -1;
}

int initramfs_read(struct file* file, void* buf, size_t len) {
    if (((struct initramfs_internal *)file->vnode->internal)->type != REGULAR_FILE) {
        printf("Read on not regular file\n");
        return -1;
    }
    struct initramfs_internal* file_node = (struct initramfs_internal *)file->vnode->internal;

    char *dest = (char*)buf;
    char *src = &(file_node->data[file->f_pos]);
    size_t i = 0;
    for (; i < len && file->f_pos < file_node->size; i++) {
        dest[i] = src[i];
        file->f_pos++;
    }
    return i;

}

int initramfs_mkdir(struct vnode* dir, struct vnode** target, const char* component_name) {
    return -1;
}


