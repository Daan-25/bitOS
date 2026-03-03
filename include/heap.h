#ifndef HEAP_H
#define HEAP_H

#include "types.h"

void heap_init(void);
void *kmalloc(size_t size);
void kfree(void *ptr);

// Debug
uint64_t heap_get_used(void);
uint64_t heap_get_free(void);

#endif
