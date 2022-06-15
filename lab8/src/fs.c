#include "vfs.h"
#include "tmpfs.h"
#include "initramfs.h"
#include "fat32.h"

struct filesystem tmpfs = {
    .name = "tmpfs",
    .setup_mount = tmpfs_setup_mount,
};

struct filesystem initramfs = {
    .name = "initramfs",
    .setup_mount = initramfs_setup_mount,
};

struct filesystem fat32 = {
    .name = "fat32",
    .setup_mount = fat32_setup_mount,
};