#ifndef VMM_H
#define VMM_H

#include "types.h"

// Page table entry flags
#define PTE_PRESENT  (1ULL << 0)
#define PTE_WRITE    (1ULL << 1)
#define PTE_USER     (1ULL << 2)
#define PTE_PS       (1ULL << 7)  // Page Size (2MB)

void vmm_init(void);
void vmm_map_page(uint64_t virt, uint64_t phys, uint64_t flags);
void vmm_unmap_page(uint64_t virt);

#endif
