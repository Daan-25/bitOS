#include "pmm.h"
#include "string.h"
#include "serial.h"

// Bitmap-based physical memory manager
// Each bit represents one 4KB page: 1 = used, 0 = free

#define BITMAP_MAX_PAGES (1024 * 1024)  // Support up to 4GB (1M pages * 4KB)
#define BITMAP_SIZE      (BITMAP_MAX_PAGES / 8)

// Place bitmap at a known location after the kernel
// Kernel is loaded at 0x10000, reserve up to 0x80000 for kernel+bitmap
#define BITMAP_ADDR 0x200000  // 2MB mark

static uint8_t *bitmap = (uint8_t *)BITMAP_ADDR;
static uint64_t total_pages = 0;
static uint64_t used_pages = 0;

static inline void bitmap_set(uint64_t page) {
    bitmap[page / 8] |= (1 << (page % 8));
}

static inline void bitmap_clear(uint64_t page) {
    bitmap[page / 8] &= ~(1 << (page % 8));
}

static inline bool bitmap_test(uint64_t page) {
    return bitmap[page / 8] & (1 << (page % 8));
}

void pmm_init(uint64_t total_memory) {
    total_pages = total_memory / PAGE_SIZE;
    if (total_pages > BITMAP_MAX_PAGES)
        total_pages = BITMAP_MAX_PAGES;

    uint64_t bitmap_bytes = total_pages / 8;

    // Mark all pages as used initially
    memset(bitmap, 0xFF, bitmap_bytes);
    used_pages = total_pages;

    // Free pages above 4MB (below that is kernel, bitmap, page tables, etc.)
    uint64_t free_start = 0x400000 / PAGE_SIZE;  // 4MB
    for (uint64_t i = free_start; i < total_pages; i++) {
        bitmap_clear(i);
        used_pages--;
    }

    serial_print("[PMM] Initialized: ");
    // Print some stats via serial
    char buf[32];
    uint64_t free_mb = (total_pages - used_pages) * PAGE_SIZE / (1024 * 1024);
    uint64_t total_mb = total_pages * PAGE_SIZE / (1024 * 1024);

    // Simple integer to string
    int pos = 0;
    uint64_t tmp = free_mb;
    if (tmp == 0) buf[pos++] = '0';
    else {
        char rev[20];
        int rp = 0;
        while (tmp > 0) { rev[rp++] = '0' + (tmp % 10); tmp /= 10; }
        while (rp > 0) buf[pos++] = rev[--rp];
    }
    buf[pos++] = '/';
    tmp = total_mb;
    if (tmp == 0) buf[pos++] = '0';
    else {
        char rev[20];
        int rp = 0;
        while (tmp > 0) { rev[rp++] = '0' + (tmp % 10); tmp /= 10; }
        while (rp > 0) buf[pos++] = rev[--rp];
    }
    buf[pos] = '\0';
    serial_print(buf);
    serial_print(" MB free\n");
}

void *pmm_alloc_page(void) {
    for (uint64_t i = 0; i < total_pages; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            used_pages++;
            uint64_t addr = i * PAGE_SIZE;
            memset((void *)addr, 0, PAGE_SIZE);
            return (void *)addr;
        }
    }
    return NULL;  // Out of memory
}

void pmm_free_page(void *page) {
    uint64_t addr = (uint64_t)page;
    uint64_t idx = addr / PAGE_SIZE;
    if (idx < total_pages && bitmap_test(idx)) {
        bitmap_clear(idx);
        used_pages--;
    }
}

uint64_t pmm_get_free_pages(void) {
    return total_pages - used_pages;
}

uint64_t pmm_get_total_pages(void) {
    return total_pages;
}
