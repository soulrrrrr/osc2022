#ifndef _INITRAMFS_H
#define _INITRAMFS_H

#include "vfs.h"

#define EOF (-1)

struct initramfs_internal {
    int type;
    char *name;
    struct vnode *vnode;
    int size;
    void *data;
};

int initramfs_register();
int initramfs_setup_mount(struct filesystem* fs, struct mount* mount);
struct vnode* initramfs_create_vnode(struct initramfs_internal* initramfs_node);

int initramfs_lookup(struct vnode* dir, struct vnode** target, const char* component_name);
int initramfs_create(struct vnode* dir, struct vnode** target, const char* component_name);
int initramfs_write(struct file* file, const void* buf, size_t len);
int initramfs_read(struct file* file, void* buf, size_t len);
int initramfs_mkdir(struct vnode* dir, struct vnode** target, const char* component_name);
#endif