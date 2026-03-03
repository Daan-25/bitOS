#include "keyboard.h"
#include "idt.h"
#include "io.h"
#include "serial.h"

#define KB_DATA_PORT 0x60
#define KB_BUF_SIZE  256

// Circular buffer for keyboard input
static char kb_buffer[KB_BUF_SIZE];
static volatile uint32_t kb_head = 0;
static volatile uint32_t kb_tail = 0;

static bool shift_pressed = false;
static bool caps_lock = false;

// US QWERTY scancode set 1 -> ASCII
static const char scancode_to_ascii[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,  'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,  '\\','z','x','c','v','b','n','m',',','.','/', 0,
    '*', 0, ' ', 0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0
};

static const char scancode_to_ascii_shift[128] = {
    0,  27, '!','@','#','$','%','^','&','*','(',')','_','+','\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
    0,  'A','S','D','F','G','H','J','K','L',':','"','~',
    0,  '|','Z','X','C','V','B','N','M','<','>','?', 0,
    '*', 0, ' ', 0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0
};

static void kb_buffer_push(char c) {
    uint32_t next = (kb_head + 1) % KB_BUF_SIZE;
    if (next != kb_tail) {
        kb_buffer[kb_head] = c;
        kb_head = next;
    }
}

static void keyboard_handler(uint64_t error_code, uint64_t int_no) {
    (void)error_code;
    (void)int_no;

    uint8_t scancode = inb(KB_DATA_PORT);

    // Key release (bit 7 set)
    if (scancode & 0x80) {
        uint8_t released = scancode & 0x7F;
        if (released == 0x2A || released == 0x36) {
            shift_pressed = false;
        }
        return;
    }

    // Special keys
    switch (scancode) {
        case 0x2A: case 0x36:  // Left/Right Shift pressed
            shift_pressed = true;
            return;
        case 0x3A:  // Caps Lock
            caps_lock = !caps_lock;
            return;
    }

    // Translate scancode to ASCII
    char c;
    if (shift_pressed) {
        c = scancode_to_ascii_shift[scancode];
    } else {
        c = scancode_to_ascii[scancode];
    }

    // Apply caps lock for letters
    if (caps_lock && c >= 'a' && c <= 'z') {
        c -= 32;
    } else if (caps_lock && c >= 'A' && c <= 'Z') {
        c += 32;
    }

    if (c) {
        kb_buffer_push(c);
    }
}

bool keyboard_has_input(void) {
    return kb_head != kb_tail;
}

char keyboard_getchar(void) {
    // Wait for input
    while (!keyboard_has_input()) {
        __asm__ volatile("hlt");
    }

    char c = kb_buffer[kb_tail];
    kb_tail = (kb_tail + 1) % KB_BUF_SIZE;
    return c;
}

// Serial port IRQ4 handler (COM1 input in -nographic mode)
static void serial_handler(uint64_t error_code, uint64_t int_no) {
    (void)error_code;
    (void)int_no;

    while (serial_has_data()) {
        char c = serial_getchar();
        if (c == '\r') c = '\n';
        kb_buffer_push(c);
    }
}

void keyboard_init(void) {
    isr_register_handler(33, keyboard_handler);  // IRQ1 = PS/2 keyboard
    isr_register_handler(36, serial_handler);    // IRQ4 = COM1 serial
}
