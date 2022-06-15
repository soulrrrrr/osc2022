#ifndef _TMPFS_H
#define _TMPFS_H

#include "vfs.h"

#define EOF (-1)

struct tmpfs_internal {
    int type;
    char name[MAX_COMPONENT_NAME_LEN];
    struct tmpfs_internal *parent;
    struct tmpfs_internal *child[MAX_ENTRIES];
    struct vnode *vnode;
    int size;
    void *data;
};

extern struct mount *rootfs;

int tmpfs_register();
int tmpfs_setup_mount(struct filesystem* fs, struct mount* mount);
struct vnode* tmpfs_create_vnode(struct tmpfs_internal* tmpfs_node);

int tmpfs_lookup(struct vnode* dir, struct vnode** target, const char* component_name);
int tmpfs_create(struct vnode* dir, struct vnode** target, const char* component_name);
int tmpfs_write(struct file* file, const void* buf, size_t len);
int tmpfs_read(struct file* file, void* buf, size_t len);
int tmpfs_mkdir(struct vnode* dir, struct vnode** target, const char* component_name);
#endif