#include "vmm.h"
#include "pmm.h"
#include "serial.h"

// Current PML4 table (set up by stage 2 bootloader at 0x1000)
static uint64_t *pml4 = (uint64_t *)0x1000;

static uint64_t *vmm_get_or_create_table(uint64_t *table, uint64_t index) {
    if (!(table[index] & PTE_PRESENT)) {
        void *new_table = pmm_alloc_page();
        if (!new_table) return NULL;
        table[index] = (uint64_t)new_table | PTE_PRESENT | PTE_WRITE;
    }
    return (uint64_t *)(table[index] & ~0xFFFULL);
}

void vmm_map_page(uint64_t virt, uint64_t phys, uint64_t flags) {
    uint64_t pml4_idx = (virt >> 39) & 0x1FF;
    uint64_t pdpt_idx = (virt >> 30) & 0x1FF;
    uint64_t pd_idx   = (virt >> 21) & 0x1FF;
    uint64_t pt_idx   = (virt >> 12) & 0x1FF;

    uint64_t *pdpt = vmm_get_or_create_table(pml4, pml4_idx);
    if (!pdpt) return;

    uint64_t *pd = vmm_get_or_create_table(pdpt, pdpt_idx);
    if (!pd) return;

    uint64_t *pt = vmm_get_or_create_table(pd, pd_idx);
    if (!pt) return;

    pt[pt_idx] = phys | flags;

    // Flush TLB for this page
    __asm__ volatile("invlpg (%0)" : : "r"(virt) : "memory");
}

void vmm_unmap_page(uint64_t virt) {
    uint64_t pml4_idx = (virt >> 39) & 0x1FF;
    uint64_t pdpt_idx = (virt >> 30) & 0x1FF;
    uint64_t pd_idx   = (virt >> 21) & 0x1FF;
    uint64_t pt_idx   = (virt >> 12) & 0x1FF;

    if (!(pml4[pml4_idx] & PTE_PRESENT)) return;
    uint64_t *pdpt = (uint64_t *)(pml4[pml4_idx] & ~0xFFFULL);

    if (!(pdpt[pdpt_idx] & PTE_PRESENT)) return;
    uint64_t *pd = (uint64_t *)(pdpt[pdpt_idx] & ~0xFFFULL);

    if (!(pd[pd_idx] & PTE_PRESENT)) return;
    uint64_t *pt = (uint64_t *)(pd[pd_idx] & ~0xFFFULL);

    pt[pt_idx] = 0;

    __asm__ volatile("invlpg (%0)" : : "r"(virt) : "memory");
}

void vmm_init(void) {
    serial_print("[VMM] Using existing page tables at 0x1000\n");
    serial_print("[VMM] Virtual memory manager ready\n");
}
