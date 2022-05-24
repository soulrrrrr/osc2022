#include "mmu.h"
#include "typedef.h"
#include "memory.h"
#include "printf.h"
#include "uart.h"

extern void memzero(void *, size_t);

void mappages(pagetable *pg_table, uint64_t va, size_t size, uint64_t pa) { //user va 轉 pa
    int pages = (size+(PAGE_SIZE-1)) / PAGE_SIZE;
    for (int i = 0; i < pages; i++) {
        // printf("Page: %d ", i);
        pte *now_pte = walk(pg_table, va + PAGE_SIZE*i, 0);
        *now_pte = (pa+PAGE_SIZE*i) | PTE_INIT;
        // printf("PTE @:");
        // uart_hex_long(now_pte);
        // printf("\n");
        // printf("Insert:");
        // uart_hex_long(*now_pte);
        // printf("\n");
    }
    return;
    // walk() size/PAGE_SIZE次
}

pte *walk(pagetable *pg_table, uint64_t va, int alloc) {
    int shift, index;
    pg_table = (pagetable *)phy_to_vir((uint64_t)pg_table);
        // printf("Start:");
        // uart_hex_long(pg_table);
        // printf("\n");
    for (int level = 3; level > 0; level--) { //PGD, PUD, PMD
        shift = 12 + 9 * level;
        index = (va & (PD_MASK << shift)) >> shift;
        // printf("%d\n", index);
        if (pg_table[index] % 4 == 3) { // is entry
            pg_table = (pagetable *)to_pfn(phy_to_vir(pg_table[index]));
            // printf("PG_TABLE: ");
            // uart_hex_long(pg_table);
            // printf("\n");
        } else {
            pg_table[index] = (to_pfn(vir_to_phy((uint64_t)malloc(PAGE_SIZE))) | PD_TABLE);
            pg_table = (pagetable *)(to_pfn(phy_to_vir(pg_table[index])));
            // printf("PG_TABLE: ");
            // uart_hex_long(pg_table);
            // printf("\n");
        }
    }
    // PTE
    shift = 12;
    index = (va & (PD_MASK << shift)) >> shift;
    return ((pte *)((uint64_t)pg_table + index*8));
}

void copypages(pagetable *parent, pagetable *child, int level) {
    if (level == 3) {
        for (int i = 0; i < 512; i++) {
            if (phy_to_vir(parent[i])) {
                pte *parent_page = (pte *)(phy_to_vir(to_pfn(parent[i])));
                child[i] = (vir_to_phy((uint64_t)malloc(PAGE_SIZE)) | PTE_INIT);
                pte *child_page = (pte *)(phy_to_vir(to_pfn(child[i])));
                for (int s = 0; s < PAGE_SIZE; s++) {
                    ((char *)child_page)[s] = ((char *)parent_page)[s];
                }
            }
        }
        return;
    }
    printf("level: %d\n", level);
    for (int i = 0; i < 512; i++) {
        if (parent[i]) {
            pagetable *parent_next_level = (pagetable *)(phy_to_vir(to_pfn(parent[i])));
            child[i] = (vir_to_phy((uint64_t)malloc(PAGE_SIZE)) | PD_TABLE);
            pagetable *child_next_level = (pagetable *)(phy_to_vir(to_pfn(child[i])));
            copypages(parent_next_level, child_next_level, level+1);
        }
    }
}


uint64_t vir_to_phy(uint64_t vir) {
    return (vir & VA_MASK);
}

uint64_t phy_to_vir(uint64_t phy) { // for kernel address
    return (phy | VA_START);
}

uint64_t to_pfn(uint64_t addr) { // to page frame
    return ((addr >> 12) << 12);
}

unsigned long va2phy_user(unsigned long va) { // user
    printf("translate va: 0x%x\n", va);
    unsigned long pgd;
    asm volatile("mrs %0, ttbr0_el1\n\t": "=r" (pgd) :: "memory");
    unsigned long pud = ((unsigned long*)(pgd+VA_START))[va>>PGD_SHIFT&(512-1)]&PAGE_MASK;
    unsigned long pmd = ((unsigned long*)(pud+VA_START))[va>>PUD_SHIFT&(512-1)]&PAGE_MASK;
    unsigned long pte = ((unsigned long*)(pmd+VA_START))[va>>PMD_SHIFT&(512-1)]&PAGE_MASK;
    unsigned long phys = ((unsigned long*)(pte+VA_START))[va>>12&(512-1)]&PAGE_MASK;
    printf("0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n", pgd, pud, pmd, pte, phys);
    return phys;
}