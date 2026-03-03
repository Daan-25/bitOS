#include "vga.h"

#define VGA_ADDR  0xB8000
#define VGA_WIDTH  80
#define VGA_HEIGHT 25

static uint16_t *vga_buffer = (uint16_t *)VGA_ADDR;
static uint8_t vga_row = 0;
static uint8_t vga_col = 0;
static uint8_t vga_attr = 0x0F; // White on black

static inline uint16_t vga_entry(char c, uint8_t attr) {
    return (uint16_t)c | ((uint16_t)attr << 8);
}

static inline uint8_t make_color(uint8_t fg, uint8_t bg) {
    return fg | (bg << 4);
}

void vga_init(void) {
    vga_attr = make_color(VGA_LIGHT_CYAN, VGA_BLACK);
    vga_clear();
}

void vga_clear(void) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = vga_entry(' ', vga_attr);
    }
    vga_row = 0;
    vga_col = 0;
}

static void vga_scroll(void) {
    // Move all rows up by one
    for (int i = 0; i < VGA_WIDTH * (VGA_HEIGHT - 1); i++) {
        vga_buffer[i] = vga_buffer[i + VGA_WIDTH];
    }
    // Clear last row
    for (int i = 0; i < VGA_WIDTH; i++) {
        vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + i] = vga_entry(' ', vga_attr);
    }
    vga_row = VGA_HEIGHT - 1;
}

void vga_putchar(char c) {
    if (c == '\n') {
        vga_col = 0;
        vga_row++;
    } else if (c == '\r') {
        vga_col = 0;
    } else if (c == '\b') {
        if (vga_col > 0) {
            vga_col--;
        } else if (vga_row > 0) {
            vga_row--;
            vga_col = VGA_WIDTH - 1;
        }
    } else if (c == '\t') {
        vga_col = (vga_col + 8) & ~7;
    } else {
        vga_buffer[vga_row * VGA_WIDTH + vga_col] = vga_entry(c, vga_attr);
        vga_col++;
    }

    if (vga_col >= VGA_WIDTH) {
        vga_col = 0;
        vga_row++;
    }
    if (vga_row >= VGA_HEIGHT) {
        vga_scroll();
    }
}

void vga_print(const char *str) {
    while (*str) {
        vga_putchar(*str++);
    }
}

void vga_set_color(uint8_t fg, uint8_t bg) {
    vga_attr = make_color(fg, bg);
}

void vga_print_color(const char *str, uint8_t fg, uint8_t bg) {
    uint8_t old_attr = vga_attr;
    vga_attr = make_color(fg, bg);
    vga_print(str);
    vga_attr = old_attr;
}
