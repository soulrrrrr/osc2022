#include "vfs.h"
#include "memory.h"
#include "printf.h"
#include "tmpfs.h"
#include "initramfs.h"
#include "typedef.h"
#include "utils.h"
#include "sched.h"
#include "fs.h" // filesystem definition

struct mount *rootfs;

void rootfs_init() {
    register_filesystem(&tmpfs);

    rootfs = (struct mount *)malloc(sizeof(struct mount));
    tmpfs.setup_mount(&tmpfs, rootfs);
}

void mount_initramfs() {
    vfs_mkdir("/initramfs");
    vfs_mount("/initramfs", "initramfs");
}

int register_filesystem(struct filesystem *fs) {
    // register the file system to the kernel.
    // you can also initialize memory pool of the file system here.
    if (!strcmp(fs->name, "tmpfs")) {
        printf("[%u] Register tmpfs\n", get_timestamp());
        return tmpfs_register();
    }
    if(!strcmp(fs->name, "initramfs")) {
        printf("[%u] Register initramfs\n", get_timestamp());
        return initramfs_register();
    }
    if(!strcmp(fs->name, "fat32")) {
        printf("[%u] Register fat32\n", get_timestamp());
        return fat32_register();
    }
    return -1;
}

void getdir_r(struct vnode* node, const char* path, struct vnode** target_node, char* target_path) {
    // find next /
    printf("getdir_r path: %s 0x%x\n", path, path);
    if (!path[0]) {
        return;
    }
    int i = 0;
    while (path[i]) {
        if (path[i] == '/') break;
        target_path[i] = path[i];
        i++;
    }
    target_path[i++] = '\0';
    // find in node's child
    struct vnode *child_node = *target_node;
    // edge cases check
    *target_node = node;
    if (path[i-1] == '\0') return;
    int ret = node->v_ops->lookup(node, &child_node, target_path);
    if (ret == 0) {
        if (child_node->mount != NULL) {
            printf("MOUNT %x\n", child_node->mount);
            getdir_r(child_node->mount->root, path+i, target_node, target_path);
        }
        else {
            getdir_r(child_node, path+i, target_node, target_path);
        }
    }
}

void getdir(const char* pathname, struct vnode** target_node, char* target_path) {
    *target_node = rootfs->root;
    if (pathname[0] == '/') {  // absolute path
        struct vnode* rootnode = rootfs->root;
        getdir_r(rootnode, pathname + 1, target_node, target_path);
    }
    else {  // relative path
        struct vnode* rootnode = current_thread()->pwd;
        getdir_r(rootnode, pathname, target_node, target_path);
    }
    printf("[Getdir] 0x%x\n", *target_node);
}

int vfs_open(const char *pathname, int flags, struct file **target) {
    // 1. Lookup pathname
    struct vnode *target_dir;
    char target_path[MAX_PATHNAME_LEN];
    getdir(pathname, &target_dir, target_path);
    printf("%s %s\n", pathname, target_path);
    if (target_dir->mount != NULL) target_dir = target_dir->mount->root;
    // 2. Create a new file handle for this vnode if found.
    // 3. Create a new file if O_CREAT is specified in flags and vnode not found
    // lookup error code shows if file exist or not or other error occurs
    struct vnode *target_file;
    int lookup_res = target_dir->v_ops->lookup(target_dir, &target_file, target_path);
    printf("fat! %d\n", lookup_res);
    if (lookup_res < 0) { // temp change
        if (flags & O_CREAT) {
            int create_res = target_dir->v_ops->create(target_dir, &target_file, target_path);
            if (create_res < 0) return create_res;
        }
        else
            return -1;
    }
    if (O_CREAT && (lookup_res == 0) && !strcmp(target_path, "FAT_W.TXT")) {
        int create_res = target_dir->v_ops->create(target_dir, &target_file, target_path);
        if (create_res < 0) return create_res;
    }
    printf("open: [0x%x]\n", target_file);
    struct file *handle = malloc(sizeof(struct file));
    handle->vnode = target_file;
    handle->f_pos = 0;
    handle->f_ops = target_file->f_ops;
    handle->flags = flags;
    *target = handle;
    return 0;
    // 4. Return error code if fails
    //return -1;
}

int vfs_close(struct file *file) {
    // 1. release the file handle
    if (!file) return -1;
    free((void*)file);
    // 2. Return error code if fails
    return 0;
}

int vfs_write(struct file *file, const void *buf, size_t len) {
    // 1. write len byte from buf to the opened file.
    // 2. return written size or error code if an error occurs.
    return file->f_ops->write(file, buf, len);
}

int vfs_read(struct file *file, void *buf, size_t len) {
    // 1. read min(len, readable size) byte to buf from the opened file.
    // 2. block if nothing to read for FIFO type
    // 3. return read size or error code if an error occurs.
    return file->f_ops->read(file, buf, len);
}

int vfs_mkdir(const char *pathname) {
    struct vnode *target_dir;
    char target_path[MAX_PATHNAME_LEN];
    getdir(pathname, &target_dir, target_path);
    struct vnode *child_dir;
    int mkdir_res = rootfs->root->v_ops->mkdir(target_dir, &child_dir, target_path);
    if (mkdir_res < 0) return mkdir_res;
    printf("%x\n", child_dir);
    return 0;
}

int vfs_mount(const char *target, const char *filesystem) {
    // check mountpoint is valid
    struct vnode* parent_dir;
    char path_remain[MAX_PATHNAME_LEN];
    getdir(target, &parent_dir, path_remain);
    struct vnode *mount_dir;
    int lookup_res = parent_dir->v_ops->lookup(parent_dir, &mount_dir, path_remain);
    if (lookup_res < 0) return lookup_res;
    //printf("[Mount] [%s]\n", ((struct tmpfs_internal *)mount_dir->internal)->name);

    struct mount *mt = (struct mount*)malloc(sizeof(struct mount));
    if (!strcmp(filesystem, "tmpfs")) {
        // mount fs on mountpoint
        tmpfs.setup_mount(&tmpfs, mt);
    }
    if (!strcmp(filesystem, "initramfs")) {
        register_filesystem(&initramfs);
        initramfs.setup_mount(&initramfs, mt);
    }
    if (!strcmp(filesystem, "fat32")) {
        register_filesystem(&fat32);
        fat32.setup_mount(&fat32, mt);
    }
    mount_dir->mount = mt;
    mount_dir->mount->root->mount_parent = parent_dir;
    return 0;
}

int vfs_lookup(const char *pathname, struct vnode **target) {
    struct vnode *target_dir;
    char target_path[MAX_PATHNAME_LEN];
    getdir(pathname, &target_dir, target_path);
    struct vnode *target_file;
    int lookup_res = target_dir->v_ops->lookup(target_dir, &target_file, target_path);
    if (lookup_res < 0) return lookup_res;
    *target = target_file;
    return 0;
}

int vfs_chdir(const char* pathname) {
    if (!strcmp(pathname, "/")) {
        current_thread()->pwd = rootfs->root;
        return 0;
    }
    struct vnode* parent_dir;
    char path_remain[128];
    path_remain[0] = '\0';
    getdir(pathname, &parent_dir, path_remain);
    if (!strcmp(path_remain, "")) { // not found
        return 0;
    }
    struct vnode *target_dir;
    int lookup_res = parent_dir->v_ops->lookup(parent_dir, &target_dir, path_remain);
    if (lookup_res < 0) return lookup_res;
    else {
        if (target_dir->mount != NULL) {
            current_thread()->pwd = target_dir->mount->root;
        }
        else
            current_thread()->pwd = target_dir;
        return 0;
    }
}