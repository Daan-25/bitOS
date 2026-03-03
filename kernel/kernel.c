#include "vga.h"
#include "serial.h"

void kmain(void) {
    serial_init();
    serial_print("\n[bitOS] Kernel started!\n");

    vga_init();

    vga_print_color("  ____  _ _    ___  ____  \n", VGA_LIGHT_GREEN, VGA_BLACK);
    vga_print_color(" | __ )(_) |_ / _ \\/ ___| \n", VGA_LIGHT_GREEN, VGA_BLACK);
    vga_print_color(" |  _ \\| | __| | | \\___ \\ \n", VGA_LIGHT_GREEN, VGA_BLACK);
    vga_print_color(" | |_) | | |_| |_| |___) |\n", VGA_LIGHT_GREEN, VGA_BLACK);
    vga_print_color(" |____/|_|\\__|\\___/|____/ \n", VGA_LIGHT_GREEN, VGA_BLACK);
    vga_print("\n");

    vga_print_color("Welcome to bitOS v0.1\n", VGA_YELLOW, VGA_BLACK);
    vga_print_color("A custom operating system built from scratch.\n\n", VGA_LIGHT_GREY, VGA_BLACK);
    vga_print("System initialized. 64-bit long mode active.\n");

    serial_print("[bitOS] VGA initialized. System ready.\n");

    // Halt
    for (;;) {
        __asm__ volatile("hlt");
    }
}
