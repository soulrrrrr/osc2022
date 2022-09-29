/*
  FUSE ssd: FUSE ioctl example
  Copyright (C) 2008       SUSE Linux Products GmbH
  Copyright (C) 2008       Tejun Heo <teheo@suse.de>
  This program can be distributed under the terms of the GNU GPLv2.
  See the file COPYING.
*/
#define FUSE_USE_VERSION 35
#include <fuse.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include "ssd_fuse_header.h"
#define SSD_NAME       "ssd_file"
enum
{
    SSD_NONE,
    SSD_ROOT,
    SSD_FILE,
};


static size_t physic_size;
static size_t logic_size;
static size_t host_write_size;
static size_t nand_write_size;

typedef union pca_rule PCA_RULE;
union pca_rule
{
    unsigned int pca;
    struct
    {
        unsigned int lba : 16;
        unsigned int nand: 16;
    } fields;
};

PCA_RULE curr_pca;
static unsigned int get_next_pca();

unsigned int* L2P,* P2L,* valid_count, free_block_number;

static int ssd_resize(size_t new_size)
{
    //set logic size to new_size
    if (new_size > NAND_SIZE_KB * 1024)
    {
        return -ENOMEM;
    }
    else
    {
        logic_size = new_size;
        return 0;
    }

}

static int ssd_expand(size_t new_size)
{
    //logic must less logic limit

    if (new_size > logic_size)
    {
        return ssd_resize(new_size);
    }

    return 0;
}

static int nand_read(char* buf, int pca)
{
    char nand_name[100];
    FILE* fptr;

    PCA_RULE my_pca;
    my_pca.pca = pca;
    snprintf(nand_name, 100, "%s/nand_%d", NAND_LOCATION, my_pca.fields.nand);

    //read
    if ( (fptr = fopen(nand_name, "r") ))
    {
        fseek( fptr, my_pca.fields.lba * 512, SEEK_SET );
        fread(buf, 1, 512, fptr);
        fclose(fptr);
    }
    else
    {
        printf("open file fail at nand read pca = %d\n", pca);
        return -EINVAL;
    }
    return 512;
}
static int nand_write(const char* buf, int pca)
{
    char nand_name[100];
    FILE* fptr;

    PCA_RULE my_pca;
    my_pca.pca = pca;
    snprintf(nand_name, 100, "%s/nand_%d", NAND_LOCATION, my_pca.fields.nand);

    //write
    if ( (fptr = fopen(nand_name, "r+")))
    {
        fseek( fptr, my_pca.fields.lba * 512, SEEK_SET );
        fwrite(buf, 1, 512, fptr);
        fclose(fptr);
        physic_size ++;
        valid_count[my_pca.fields.nand]++;
    }
    else
    {
        printf("open file fail at nand (%s) write pca = %d, return %d\n", nand_name, pca, -EINVAL);
        return -EINVAL;
    }

    nand_write_size += 512;
    return 512;
}

static int nand_erase(int block_index)
{
    //printf("[NAND ERASE] %d\n", block_index);
    char nand_name[100];
    FILE* fptr;
    snprintf(nand_name, 100, "%s/nand_%d", NAND_LOCATION, block_index);
    fptr = fopen(nand_name, "w");
    if (fptr == NULL)
    {
        printf("erase nand_%d fail", block_index);
        return 0;
    }
    fclose(fptr);
    valid_count[block_index] = FREE_BLOCK;
    free_block_number++;
    return 1;
}

static unsigned int get_next_block()
{   
    for (int i = 0; i < PHYSICAL_NAND_NUM; i++)
    {
        if (valid_count[(curr_pca.fields.nand + i) % PHYSICAL_NAND_NUM] == FREE_BLOCK)
        {
            curr_pca.fields.nand = (curr_pca.fields.nand + i) % PHYSICAL_NAND_NUM;
            curr_pca.fields.lba = 0;
            free_block_number--;
            valid_count[curr_pca.fields.nand] = 0;
            return curr_pca.pca;
        }
    }
    return OUT_OF_BLOCK;
}
static unsigned int get_next_pca()
{
    if (curr_pca.pca == INVALID_PCA)
    {
        //init
        curr_pca.pca = 0;
        valid_count[0] = 0;
        free_block_number--;
        return curr_pca.pca;
    }

    if(curr_pca.fields.lba == 9)
    {
        int temp = get_next_block();
        if (temp == OUT_OF_BLOCK)
        {
            return OUT_OF_BLOCK;
        }
        else if(temp == -EINVAL)
        {
            return -EINVAL;
        }
        else
        {
            return temp;
        }
    }
    else
    {
        curr_pca.fields.lba += 1;
    }
    return curr_pca.pca;

}

//----------------------------------------------------------------

//----------------------------------------------------------------

static int ftl_read( char* buf, size_t lba)
{
    // TODO
    // 呼叫 nand_read
    nand_read(buf, L2P[lba]);
    return 0;
}

static int ftl_write(const char* buf, size_t lba_range, size_t lba)
{
    // TODO
    // 如果不是 INVALID_PCA 表示此 logic page 已被寫過，
    // 舊的 physical 要清除 P2L 紀錄
    if (L2P[lba] != INVALID_PCA) {
        valid_count[L2P[lba]>>16]--;
        P2L[(L2P[lba]>>16)*10+(L2P[lba]&0xffff)] = INVALID_LBA;
    }
    // 更新 L2P, P2L 表
    PCA_RULE temp;
    temp.pca = get_next_pca();
    L2P[lba] = temp.pca;
    P2L[temp.fields.nand*10 + temp.fields.lba] = lba;
    // 呼叫 nand_write
    nand_write(buf, L2P[lba]);
    return 0;
}

//----------------------------------------------------------------
char gcbuf[512];

static void gc() {
    // for (int i = 0; i < 13; i++) {
    //     printf("%x ", valid_count[i]);
    // }
    // printf("\n");
    
    // free 1 block
    int t = 1;
    while(t-- > 0) {
    //while(free_block_number < 1) {

        // 找移的 page 最少的 nand 
        int del = -1;
        unsigned int pages = 11;
        for (int i = 0; i < 13; i++) {
            if (i == curr_pca.fields.nand) continue;
            if (valid_count[i] < pages) {
                del = i;
                pages = valid_count[i];
            }
        }
        if (del == -1 || pages == FREE_BLOCK) {
            break;
        }
        //printf("[GC] %d ,%d pages\n", del, 10-pages);
        //char *buf = (char *)malloc(512);
        for (int i = 0; i < 10; i++) {

            // 不是 INVALID_LBA 的 page 要移動
            if (P2L[del*10+i] != INVALID_LBA) {
                ftl_read(gcbuf, P2L[del*10+i]);
                ftl_write(gcbuf, 0, P2L[del*10+i]);
                P2L[del*10+i] = INVALID_LBA;
            }
        }
        //free(buf);

        // 都移動完了就 erase 該 nand
        nand_erase(del);
    }

    // for (int i = 0; i < 13; i++) {
    //     printf("%x ", valid_count[i]);
    // }
    // printf("\n");
    // fflush(stdout);
    return;
}
//----------------------------------------------------------------

static int ssd_file_type(const char* path)
{
    if (strcmp(path, "/") == 0)
    {
        return SSD_ROOT;
    }
    if (strcmp(path, "/" SSD_NAME) == 0)
    {
        return SSD_FILE;
    }
    return SSD_NONE;
}
static int ssd_getattr(const char* path, struct stat* stbuf,
                       struct fuse_file_info* fi)
{
    (void) fi;
    stbuf->st_uid = getuid();
    stbuf->st_gid = getgid();
    stbuf->st_atime = stbuf->st_mtime = time(NULL);
    switch (ssd_file_type(path))
    {
        case SSD_ROOT:
            stbuf->st_mode = S_IFDIR | 0755;
            stbuf->st_nlink = 2;
            break;
        case SSD_FILE:
            stbuf->st_mode = S_IFREG | 0644;
            stbuf->st_nlink = 1;
            stbuf->st_size = logic_size;
            break;
        case SSD_NONE:
            return -ENOENT;
    }
    return 0;
}
static int ssd_open(const char* path, struct fuse_file_info* fi)
{
    (void) fi;
    if (ssd_file_type(path) != SSD_NONE)
    {
        return 0;
    }
    return -ENOENT;
}
static int ssd_do_read(char* buf, size_t size, off_t offset)
{
    int tmp_lba, tmp_lba_range, rst ;
    char* tmp_buf;

    //off limit
    if ((offset ) >= logic_size)
    {
        return 0;
    }
    if ( size > logic_size - offset)
    {
        //is valid data section
        size = logic_size - offset;
    }

    tmp_lba = offset / 512;
    tmp_lba_range = (offset + size - 1) / 512 - (tmp_lba) + 1;
    tmp_buf = (char *)calloc(tmp_lba_range * 512, sizeof(char));

    for (int i = 0; i < tmp_lba_range; i++) {
        // TODO
        // 呼叫 ftl_read 依序寫進 tmp_buf
        ftl_read(tmp_buf+(512*i), tmp_lba+i);
    }

    memcpy(buf, tmp_buf + (offset % 512), size);

    
    free(tmp_buf);
    return size;
}
static int ssd_read(const char* path, char* buf, size_t size,
                    off_t offset, struct fuse_file_info* fi)
{
    (void) fi;
    if (ssd_file_type(path) != SSD_FILE)
    {
        return -EINVAL;
    }
    return ssd_do_read(buf, size, offset);
}

char tmp_buf[512];
static int ssd_do_write(const char* buf, size_t size, off_t offset)
{
    int tmp_lba, tmp_lba_range, process_size;
    int idx, curr_size, remain_size, rst;
    //char* tmp_buf;

    host_write_size += size;
    if (ssd_expand(offset + size) != 0)
    {
        return -ENOMEM;
    }

    tmp_lba = offset / 512;
    tmp_lba_range = (offset + size - 1) / 512 - (tmp_lba) + 1;

    process_size = 0; // 這次 for 要處理的 size
    remain_size = size; // 剩下的 size
    curr_size = 0; // 目前已處理的 size 

    //tmp_buf = (char *)malloc(512);
    for (idx = 0; idx < tmp_lba_range; idx++)
    {
        // TODO
        // 如果沒有 free block 以及要移的 block 跟剩下的 page 相等時做 garbage collection
        // if (free_block_number == 0) {
        //     printf("------\n");
        //     gc();
        // } 
        if (free_block_number == 0) {
            int del = -1;
            unsigned int pages = 11;
            for (int i = 0; i < 13; i++) {
                if (i == curr_pca.fields.nand) continue;
                if (valid_count[i] < pages) {
                    del = i;
                    pages = valid_count[i];
                }
            }
            if (del == -1 || pages == FREE_BLOCK) {
                break;
            }
            if (9-curr_pca.fields.lba == pages) {
                //printf("-----gc\n");
                gc();
            }

        }
            
        
        // read
        ftl_read(tmp_buf, tmp_lba+idx);
        
        // modify
        if (idx == 0) {
            process_size = 512 - (offset % 512);
            if (size < process_size) process_size = size; // size < 512
            memcpy((char *)tmp_buf+(offset % 512), (char *)buf+curr_size, process_size);
        }
        else if (idx == tmp_lba_range-1) {
            process_size = remain_size;
            memcpy((char *)tmp_buf, (char *)buf+curr_size, process_size);
        }
        else {
            process_size = 512;
            memcpy((char *)tmp_buf, (char *)buf+curr_size, process_size);
        }
        curr_size += process_size;
        remain_size -= process_size;
        //printf("[process] %d [curr] %d [remain] %d\n", process_size, curr_size, remain_size);
        
        // write
        ftl_write(tmp_buf, 0, tmp_lba+idx);
    }
    //free(tmp_buf);
    return size;
}
static int ssd_write(const char* path, const char* buf, size_t size,
                     off_t offset, struct fuse_file_info* fi)
{

    (void) fi;
    if (ssd_file_type(path) != SSD_FILE)
    {
        return -EINVAL;
    }
    return ssd_do_write(buf, size, offset);
}
static int ssd_truncate(const char* path, off_t size,
                        struct fuse_file_info* fi)
{
    (void) fi;
    memset(L2P, INVALID_PCA, sizeof(int) * LOGICAL_NAND_NUM * PAGE_PER_BLOCK);
    memset(P2L, INVALID_LBA, sizeof(int) * PHYSICAL_NAND_NUM * PAGE_PER_BLOCK);
    memset(valid_count, FREE_BLOCK, sizeof(int) * PHYSICAL_NAND_NUM);
    curr_pca.pca = INVALID_PCA;
    free_block_number = PHYSICAL_NAND_NUM;
    if (ssd_file_type(path) != SSD_FILE)
    {
        return -EINVAL;
    }

    return ssd_resize(size);
}
static int ssd_readdir(const char* path, void* buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info* fi,
                       enum fuse_readdir_flags flags)
{
    (void) fi;
    (void) offset;
    (void) flags;
    if (ssd_file_type(path) != SSD_ROOT)
    {
        return -ENOENT;
    }
    filler(buf, ".", NULL, 0, (enum fuse_fill_dir_flags)0);
    filler(buf, "..", NULL, 0, (enum fuse_fill_dir_flags)0);
    filler(buf, SSD_NAME, NULL, 0, (enum fuse_fill_dir_flags)0);
    return 0;
}
static int ssd_ioctl(const char* path, unsigned int cmd, void* arg,
                     struct fuse_file_info* fi, unsigned int flags, void* data)
{

    if (ssd_file_type(path) != SSD_FILE)
    {
        return -EINVAL;
    }
    if (flags & FUSE_IOCTL_COMPAT)
    {
        return -ENOSYS;
    }
    switch (cmd)
    {
        case SSD_GET_LOGIC_SIZE:
            *(size_t*)data = logic_size;
            return 0;
        case SSD_GET_PHYSIC_SIZE:
            *(size_t*)data = physic_size;
            return 0;
        case SSD_GET_WA:
            *(double*)data = (double)nand_write_size / (double)host_write_size;
            return 0;
    }
    return -EINVAL;
}

static const struct fuse_operations ssd_oper =
{
    .getattr        = ssd_getattr,
    .truncate       = ssd_truncate,
    .open           = ssd_open,
    .read           = ssd_read,
    .write          = ssd_write,
    .readdir        = ssd_readdir,
    .ioctl          = ssd_ioctl,
};

int main(int argc, char* argv[])
{
    int idx;
    char nand_name[100];
    physic_size = 0;
    logic_size = 0;
    curr_pca.pca = INVALID_PCA;
    free_block_number = PHYSICAL_NAND_NUM;

    L2P = (unsigned int *)malloc(LOGICAL_NAND_NUM * PAGE_PER_BLOCK * sizeof(int));
    memset(L2P, INVALID_PCA, sizeof(int) * LOGICAL_NAND_NUM * PAGE_PER_BLOCK);
    P2L = (unsigned int *)malloc(PHYSICAL_NAND_NUM * PAGE_PER_BLOCK * sizeof(int));
    memset(P2L, INVALID_LBA, sizeof(int) * PHYSICAL_NAND_NUM * PAGE_PER_BLOCK);
    valid_count = (unsigned int *)malloc(PHYSICAL_NAND_NUM * sizeof(int));
    memset(valid_count, FREE_BLOCK, sizeof(int) * PHYSICAL_NAND_NUM);

    //create nand file
    for (idx = 0; idx < PHYSICAL_NAND_NUM; idx++)
    {
        FILE* fptr;
        snprintf(nand_name, 100, "%s/nand_%d", NAND_LOCATION, idx);
        fptr = fopen(nand_name, "w");
        if (fptr == NULL)
        {
            printf("open fail");
        }
        fclose(fptr);
    }
    return fuse_main(argc, argv, &ssd_oper, NULL);
}