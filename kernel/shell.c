#include "shell.h"
#include "vga.h"
#include "serial.h"
#include "keyboard.h"
#include "string.h"
#include "pmm.h"
#include "heap.h"

#define CMD_BUF_SIZE 256
#define MAX_ARGS     16

static char cmd_buf[CMD_BUF_SIZE];
static uint32_t cmd_len = 0;

// Forward declarations for commands
static void cmd_help(int argc, char **argv);
static void cmd_clear(int argc, char **argv);
static void cmd_echo(int argc, char **argv);
static void cmd_info(int argc, char **argv);
static void cmd_meminfo(int argc, char **argv);
static void cmd_reboot(int argc, char **argv);

typedef struct {
    const char *name;
    const char *desc;
    void (*func)(int argc, char **argv);
} shell_cmd_t;

static shell_cmd_t commands[] = {
    { "help",    "Show available commands",   cmd_help    },
    { "clear",   "Clear the screen",          cmd_clear   },
    { "echo",    "Print text to screen",      cmd_echo    },
    { "info",    "Show system information",   cmd_info    },
    { "meminfo", "Show memory statistics",    cmd_meminfo },
    { "reboot",  "Reboot the system",         cmd_reboot  },
    { NULL, NULL, NULL }
};

static void print_prompt(void) {
    vga_print_color("bitos", VGA_LIGHT_GREEN, VGA_BLACK);
    vga_print_color("> ", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_set_color(VGA_WHITE, VGA_BLACK);

    serial_print("bitos> ");
}

static void parse_and_execute(void) {
    // Skip leading spaces
    char *input = cmd_buf;
    while (*input == ' ') input++;

    if (*input == '\0') return;

    // Split into argv
    char *argv[MAX_ARGS];
    int argc = 0;
    char *p = input;

    while (*p && argc < MAX_ARGS) {
        while (*p == ' ') p++;
        if (*p == '\0') break;
        argv[argc++] = p;
        while (*p && *p != ' ') p++;
        if (*p) *p++ = '\0';
    }

    if (argc == 0) return;

    // Find and execute command
    for (int i = 0; commands[i].name != NULL; i++) {
        if (strcmp(argv[0], commands[i].name) == 0) {
            commands[i].func(argc, argv);
            return;
        }
    }

    vga_print_color("Unknown command: ", VGA_LIGHT_RED, VGA_BLACK);
    vga_print(argv[0]);
    vga_print("\nType 'help' for available commands.\n");

    serial_print("Unknown command: ");
    serial_print(argv[0]);
    serial_print("\n");
}

// === Shell commands ===

static void cmd_help(int argc, char **argv) {
    (void)argc; (void)argv;

    vga_print_color("\nAvailable commands:\n", VGA_YELLOW, VGA_BLACK);
    serial_print("\nAvailable commands:\n");

    for (int i = 0; commands[i].name != NULL; i++) {
        vga_print("  ");
        vga_print_color(commands[i].name, VGA_LIGHT_CYAN, VGA_BLACK);

        // Pad to 10 chars
        int pad = 10 - (int)strlen(commands[i].name);
        while (pad-- > 0) vga_putchar(' ');

        vga_print_color(commands[i].desc, VGA_LIGHT_GREY, VGA_BLACK);
        vga_putchar('\n');

        serial_print("  ");
        serial_print(commands[i].name);
        serial_print("  ");
        serial_print(commands[i].desc);
        serial_print("\n");
    }
    vga_putchar('\n');
    serial_print("\n");
}

static void cmd_clear(int argc, char **argv) {
    (void)argc; (void)argv;
    vga_clear();
}

static void cmd_echo(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (i > 1) {
            vga_putchar(' ');
            serial_putchar(' ');
        }
        vga_print(argv[i]);
        serial_print(argv[i]);
    }
    vga_putchar('\n');
    serial_print("\n");
}

static void print_size(uint64_t bytes) {
    char buf[32];
    if (bytes >= 1024 * 1024) {
        utoa(bytes / (1024 * 1024), buf, 10);
        vga_print(buf); vga_print(" MB");
        serial_print(buf); serial_print(" MB");
    } else if (bytes >= 1024) {
        utoa(bytes / 1024, buf, 10);
        vga_print(buf); vga_print(" KB");
        serial_print(buf); serial_print(" KB");
    } else {
        utoa(bytes, buf, 10);
        vga_print(buf); vga_print(" B");
        serial_print(buf); serial_print(" B");
    }
}

static void cmd_meminfo(int argc, char **argv) {
    (void)argc; (void)argv;
    char buf[32];

    vga_print_color("\n=== Memory Info ===\n", VGA_YELLOW, VGA_BLACK);
    serial_print("\n=== Memory Info ===\n");

    // Physical memory
    uint64_t free_pages = pmm_get_free_pages();
    uint64_t total_pages = pmm_get_total_pages();
    uint64_t used_pg = total_pages - free_pages;

    vga_print("  Physical: ");
    serial_print("  Physical: ");
    print_size(used_pg * PAGE_SIZE);
    vga_print(" used / ");
    serial_print(" used / ");
    print_size(total_pages * PAGE_SIZE);
    vga_print(" total (");
    serial_print(" total (");
    print_size(free_pages * PAGE_SIZE);
    vga_print(" free)\n");
    serial_print(" free)\n");

    utoa(free_pages, buf, 10);
    vga_print("  Pages:    "); vga_print(buf);
    serial_print("  Pages:    "); serial_print(buf);
    vga_print(" free / ");
    serial_print(" free / ");
    utoa(total_pages, buf, 10);
    vga_print(buf); vga_print(" total\n");
    serial_print(buf); serial_print(" total\n");

    // Heap
    vga_print("  Heap:     ");
    serial_print("  Heap:     ");
    print_size(heap_get_used());
    vga_print(" used / ");
    serial_print(" used / ");
    print_size(heap_get_used() + heap_get_free());
    vga_print(" total\n\n");
    serial_print(" total\n\n");
}

static void cmd_info(int argc, char **argv) {
    (void)argc; (void)argv;

    vga_print_color("\n=== bitOS System Info ===\n", VGA_YELLOW, VGA_BLACK);
    vga_print("  OS:       bitOS v0.3\n");
    vga_print("  Arch:     x86_64 (long mode)\n");
    vga_print("  Kernel:   loaded at 0x10000\n");
    vga_print("  Video:    VGA text mode 80x25\n");
    vga_print("  Keyboard: PS/2 + Serial\n");
    vga_print("  Serial:   COM1 (0x3F8)\n");
    vga_print("  Memory:   PMM + VMM + Heap\n\n");

    serial_print("\n=== bitOS System Info ===\n");
    serial_print("  OS:       bitOS v0.3\n");
    serial_print("  Arch:     x86_64 (long mode)\n");
    serial_print("  Kernel:   loaded at 0x10000\n");
    serial_print("  Video:    VGA text mode 80x25\n");
    serial_print("  Keyboard: PS/2 + Serial\n");
    serial_print("  Serial:   COM1 (0x3F8)\n");
    serial_print("  Memory:   PMM + VMM + Heap\n\n");
}

static void cmd_reboot(int argc, char **argv) {
    (void)argc; (void)argv;
    vga_print("Rebooting...\n");
    serial_print("Rebooting...\n");

    // Triple fault to reboot: load a zero-length IDT and trigger interrupt
    struct { uint16_t limit; uint64_t base; } __attribute__((packed)) null_idt = { 0, 0 };
    __asm__ volatile("lidt %0" : : "m"(null_idt));
    __asm__ volatile("int $0x03");
}

// === Main shell loop ===

void shell_init(void) {
    vga_print_color("bitOS Shell v0.1\n", VGA_YELLOW, VGA_BLACK);
    vga_print("Type 'help' for available commands.\n\n");
    serial_print("bitOS Shell v0.1\nType 'help' for available commands.\n\n");
}

void shell_run(void) {
    shell_init();
    print_prompt();

    while (1) {
        char c = keyboard_getchar();

        if (c == '\n') {
            vga_putchar('\n');
            serial_print("\n");
            cmd_buf[cmd_len] = '\0';
            parse_and_execute();
            cmd_len = 0;
            memset(cmd_buf, 0, CMD_BUF_SIZE);
            print_prompt();
        } else if (c == '\b') {
            if (cmd_len > 0) {
                cmd_len--;
                cmd_buf[cmd_len] = '\0';
                // Erase character on VGA
                vga_putchar('\b');
                vga_putchar(' ');
                vga_putchar('\b');
                serial_print("\b \b");
            }
        } else if (cmd_len < CMD_BUF_SIZE - 1) {
            cmd_buf[cmd_len++] = c;
            vga_putchar(c);
            serial_putchar(c);
        }
    }
}
