#include "heap.h"
#include "string.h"
#include "serial.h"

// Simple first-fit heap allocator
// Heap lives in a fixed region starting at 16MB

#define HEAP_START 0x1000000   // 16MB
#define HEAP_SIZE  0x1000000   // 16MB heap size (16MB - 32MB range)

// Block header: stored before each allocated/free block
typedef struct block_header {
    uint64_t size;                  // Size of usable data (excludes header)
    bool     free;                  // Is this block free?
    struct block_header *next;      // Next block in list
} block_header_t;

#define HEADER_SIZE sizeof(block_header_t)

static block_header_t *heap_head = NULL;
static uint64_t total_allocated = 0;

void heap_init(void) {
    heap_head = (block_header_t *)HEAP_START;
    heap_head->size = HEAP_SIZE - HEADER_SIZE;
    heap_head->free = true;
    heap_head->next = NULL;
    total_allocated = 0;

    serial_print("[HEAP] Initialized: 16 MB at 0x1000000\n");
}

// Split a block if it's large enough
static void split_block(block_header_t *block, uint64_t size) {
    uint64_t remaining = block->size - size - HEADER_SIZE;
    if (remaining < 32) return;  // Don't split tiny blocks

    block_header_t *new_block = (block_header_t *)((uint8_t *)block + HEADER_SIZE + size);
    new_block->size = remaining;
    new_block->free = true;
    new_block->next = block->next;

    block->size = size;
    block->next = new_block;
}

void *kmalloc(size_t size) {
    if (size == 0) return NULL;

    // Align to 16 bytes
    size = (size + 15) & ~15ULL;

    block_header_t *current = heap_head;
    while (current) {
        if (current->free && current->size >= size) {
            split_block(current, size);
            current->free = false;
            total_allocated += current->size;
            return (void *)((uint8_t *)current + HEADER_SIZE);
        }
        current = current->next;
    }

    serial_print("[HEAP] Out of memory!\n");
    return NULL;
}

// Merge adjacent free blocks
static void coalesce(void) {
    block_header_t *current = heap_head;
    while (current && current->next) {
        if (current->free && current->next->free) {
            current->size += HEADER_SIZE + current->next->size;
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
}

void kfree(void *ptr) {
    if (!ptr) return;

    block_header_t *block = (block_header_t *)((uint8_t *)ptr - HEADER_SIZE);
    if (block->free) return;  // Double free protection

    total_allocated -= block->size;
    block->free = true;
    coalesce();
}

uint64_t heap_get_used(void) {
    return total_allocated;
}

uint64_t heap_get_free(void) {
    return HEAP_SIZE - HEADER_SIZE - total_allocated;
}
