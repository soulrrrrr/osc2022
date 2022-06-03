#include "memory.h"
#include "vfs.h"
#include "tmpfs.h"
#include "printf.h"
#include "utils.h"

struct vnode_operations* tmpfs_v_ops;
struct file_operations* tmpfs_f_ops;

int tmpfs_register() {
    tmpfs_v_ops = (struct vnode_operations*)malloc(sizeof(struct vnode_operations));
    tmpfs_v_ops->lookup = &tmpfs_lookup;
    tmpfs_v_ops->create = &tmpfs_create;
    tmpfs_v_ops->mkdir = &tmpfs_mkdir;
    tmpfs_f_ops = (struct file_operations*)malloc(sizeof(struct file_operations));
    tmpfs_f_ops->write = &tmpfs_write;
    tmpfs_f_ops->read = &tmpfs_read;
    return 0;
}

int tmpfs_setup_mount(struct filesystem* fs, struct mount* mount) {
    struct tmpfs_internal* tmpfs_root = (struct tmpfs_internal*)malloc(sizeof(struct tmpfs_internal));
    tmpfs_root->type = DIRECTORY;
    strcpy(tmpfs_root->name, "/");
    tmpfs_root->parent = NULL;
    struct node *root_vnode = tmpfs_create_vnode(tmpfs_root);
    mount->root = root_vnode;
    mount->fs = fs;
    tmpfs_root->vnode = root_vnode;
    return 0;
}

struct vnode* tmpfs_create_vnode(struct tmpfs_internal* tmpfs_node) {
    struct vnode* vnode = (struct vnode*)malloc(sizeof(struct vnode));
    vnode->f_ops = tmpfs_f_ops;
    vnode->v_ops = tmpfs_v_ops;
    vnode->internal = tmpfs_node;
    return vnode;
}

int tmpfs_create(struct vnode* dir, struct vnode** target, const char* component_name) {
    // create tmpfs internal structure
    struct tmpfs_internal* file_node = (struct tmpfs_internal*)malloc(sizeof(struct tmpfs_internal));
    file_node->type = REGULAR_FILE;
    strcpy(file_node->name, component_name);
    file_node->parent = (struct tmpfs_internal *)(dir->internal);
    for (int i = 0; i < MAX_ENTRIES; i++) {
        if (!file_node->parent->child[i]) {
            file_node->parent->child[i] = file_node;
            break;
        }
    }
    file_node->vnode = tmpfs_create_vnode(file_node);
    file_node->data = malloc(PAGE_SIZE);

    *target = file_node->vnode;
    return 0;
}

// vnode operations
int tmpfs_lookup(struct vnode* dir, struct vnode** target, const char* component_name) {
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
        }
        struct tmpfs_internal *t = ((struct tmpfs_internal *)dir->internal)->parent;
        if (!t) return 0;
        *target = ((struct tmpfs_internal *)dir->internal)->parent->vnode;
        return 0;
    }
    // search component_name in dir
    if (dir->mount != NULL)
        dir = dir->mount->root;
    for (int i = 0; i < MAX_ENTRIES; i++) {
        struct tmpfs_internal* file_node = ((struct tmpfs_internal*)dir->internal)->child[i];
        if ((file_node != NULL) & !strcmp(file_node->name, component_name)) {
            *target = file_node->vnode;
            printf("[lookup] 0x%x\n", *target);
            return 0;
        }
    }
    *target = NULL;
    return -1;
}

int tmpfs_write(struct file* file, const void* buf, size_t len) {
    if (((struct tmpfs_internal *)file->vnode->internal)->type != REGULAR_FILE) {
        printf("Write on not regular file\n");
        return -1;
    }

    struct tmpfs_internal* file_node = (struct tmpfs_internal *)file->vnode->internal;

    char *dest = &(file_node->data[file->f_pos]);
    char *src = (char*)buf;
    size_t i = 0;
    for (; i < len; i++) {
        dest[i] = src[i];
    }
    //dest[i] = EOF;
    return i;
}

int tmpfs_read(struct file* file, void* buf, size_t len) {
    if (((struct tmpfs_internal *)file->vnode->internal)->type != REGULAR_FILE) {
        printf("Read on not regular file\n");
        return -1;
    }
    struct tmpfs_internal* file_node = (struct tmpfs_internal *)file->vnode->internal;

    char *dest = (char*)buf;
    char *src = &(file_node->data[file->f_pos]);
    size_t i = 0;
    for (; i < len && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    file->f_pos += i;
    return i;

}

int tmpfs_mkdir(struct vnode* dir, struct vnode** target, const char* component_name) {
    // create tmpfs internal structure
    struct tmpfs_internal* dir_node = (struct tmpfs_internal*)malloc(sizeof(struct tmpfs_internal));
    dir_node->type = DIRECTORY;
    strcpy(dir_node->name, component_name);
    dir_node->parent = (struct tmpfs_internal *)(dir->internal);
    for (int i = 0; i < MAX_ENTRIES; i++) {
        if (!dir_node->parent->child[i]) {
            dir_node->parent->child[i] = dir_node;
            break;
        }
    }
    dir_node->vnode = tmpfs_create_vnode(dir_node);
    *target = dir_node->vnode;
    return 0;
}


