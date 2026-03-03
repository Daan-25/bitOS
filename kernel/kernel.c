#include "vga.h"
#include "serial.h"
#include "idt.h"
#include "keyboard.h"

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

    vga_print_color("Welcome to bitOS v0.2\n", VGA_YELLOW, VGA_BLACK);
    vga_print_color("64-bit long mode active.\n\n", VGA_LIGHT_GREY, VGA_BLACK);

    // Initialize IDT and interrupts
    serial_print("[bitOS] Setting up IDT...\n");
    idt_init();
    serial_print("[bitOS] IDT loaded, interrupts enabled.\n");

    // Initialize keyboard
    keyboard_init();
    serial_print("[bitOS] Keyboard driver ready.\n");

    vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    vga_print("Interrupts enabled. Keyboard ready.\n");
    vga_print("Type something:\n\n");
    vga_set_color(VGA_WHITE, VGA_BLACK);

    // Echo keyboard input
    while (1) {
        char c = keyboard_getchar();
        vga_putchar(c);
        serial_putchar(c);
    }
}
