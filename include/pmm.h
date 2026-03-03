#ifndef PMM_H
#define PMM_H

#include "types.h"

#define PAGE_SIZE 4096

void pmm_init(uint64_t total_memory);
void *pmm_alloc_page(void);
void pmm_free_page(void *page);
uint64_t pmm_get_free_pages(void);
uint64_t pmm_get_total_pages(void);

#endif
