#include "vga.h"
#include "serial.h"
#include "idt.h"
#include "keyboard.h"
#include "pmm.h"
#include "vmm.h"
#include "heap.h"
#include "shell.h"

#define DEFAULT_MEMORY (128 * 1024 * 1024)  // Assume 128MB (QEMU default)

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

    // Initialize IDT and interrupts
    serial_print("[bitOS] Setting up IDT...\n");
    idt_init();
    serial_print("[bitOS] IDT loaded, interrupts enabled.\n");

    // Initialize keyboard
    keyboard_init();
    serial_print("[bitOS] Keyboard driver ready.\n");

    // Initialize memory management
    pmm_init(DEFAULT_MEMORY);
    vmm_init();
    heap_init();

    // Start shell
    shell_run();
}
