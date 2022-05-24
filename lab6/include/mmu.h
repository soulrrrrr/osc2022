#ifndef _MMU_H
#define _MMU_H

//#define KERNEL_VIRT_BASE 0xFFFF000000000000
#define KERNEL_VIRT_BASE 0x0
// tcr
// https://developer.arm.com/documentation/ddi0595/2021-06/AArch64-Registers/TCR-EL1--Translation-Control-Register--EL1-
#define TCR_CONFIG_REGION_48bit (((64 - 48) << 0) | ((64 - 48) << 16))
#define TCR_CONFIG_4KB ((0b00 << 14) |  (0b10 << 30))
#define TCR_CONFIG_DEFAULT (TCR_CONFIG_REGION_48bit | TCR_CONFIG_4KB)

// mair
// https://developer.arm.com/documentation/ddi0595/2021-06/AArch64-Registers/MAIR-EL1--Memory-Attribute-Indirection-Register--EL1-
#define MAIR_DEVICE_nGnRnE 0b00000000
#define MAIR_NORMAL_NOCACHE 0b01000100
#define MAIR_IDX_DEVICE_nGnRnE 0
#define MAIR_IDX_NORMAL_NOCACHE 1

// identity paging
#define PD_TABLE 0b11
#define PD_ENTRY 0b11
#define PD_BLOCK 0b01
#define PD_ACCESS (1 << 10)
#define PD_USER_ACCESS (0b01 << 6)
#define BOOT_PGD_ATTR PD_TABLE
#define BOOT_PUD_ATTR (PD_ACCESS | (MAIR_IDX_DEVICE_nGnRnE << 2) | PD_BLOCK)

#define PGD_INIT (PD_ACCESS | PD_USER_ACCESS | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_BLOCK)
#define PTE_INIT (PD_ACCESS | PD_USER_ACCESS | (MAIR_IDX_DEVICE_nGnRnE << 2) | PD_ENTRY)

#define VA_START    0xFFFF000000000000
#define VA_MASK     0x0000FFFFFFFFFFFF
#define LOW_MEMORY  0x80000
#define PD_MASK                 0x1FFUL // 9 bit
#define PGF_MASK    0xFFFFFFFFF000

#define PAGE_MASK			      0xfffffffffffff000 // va2phy_user
#define PGD_SHIFT   (12 + 3*9)
#define PUD_SHIFT   (12 + 2*9)
#define PMD_SHIFT   (12 + 9)

// user space constants
#define USER_PC         0x0000000000000000
#define USER_SP         0x0000FFFFFFFFF000
#define USER_STACK_LOW  0x0000FFFFFFFFB000
#define USER_STACK_SIZE 0x4000

#ifndef __ASSEMBLY__
#include "typedef.h"
// page table & entry
typedef unsigned long pte;
typedef unsigned long pagetable;
void mappages(pagetable *pg_table, uint64_t va, uint64_t size, uint64_t pa);
pte *walk(pagetable *pg_table, uint64_t va, int alloc);
void copypages(pagetable *parent, pagetable *child, int level);
uint64_t vir_to_phy(uint64_t vir);
uint64_t phy_to_vir(uint64_t phy);
uint64_t to_pfn(uint64_t addr);
unsigned long va2phy_user(unsigned long va);
#endif

#endif