#include "timer.h"
#include "idt.h"
#include "io.h"
#include "serial.h"

#define PIT_CMD  0x43
#define PIT_CH0  0x40

// PIT base frequency
#define PIT_FREQ 1193182

static volatile uint64_t ticks = 0;

static void timer_handler(uint64_t error_code, uint64_t int_no) {
    (void)error_code;
    (void)int_no;
    ticks++;
}

void timer_init(void) {
    // Register IRQ0 handler
    isr_register_handler(32, timer_handler);

    // Configure PIT channel 0: rate generator, lo/hi byte
    uint16_t divisor = PIT_FREQ / TIMER_HZ;
    outb(PIT_CMD, 0x36);          // Channel 0, lo/hi, rate generator
    outb(PIT_CH0, divisor & 0xFF);
    outb(PIT_CH0, (divisor >> 8) & 0xFF);

    // Unmask IRQ0 on PIC (currently masked)
    uint8_t mask = inb(0x21);
    mask &= ~(1 << 0);           // Clear bit 0 to unmask IRQ0
    outb(0x21, mask);

    serial_print("[TIMER] PIT initialized at 100 Hz\n");
}

uint64_t timer_get_ticks(void) {
    return ticks;
}

uint64_t timer_get_seconds(void) {
    return ticks / TIMER_HZ;
}

void timer_sleep(uint64_t ms) {
    uint64_t target = ticks + (ms * TIMER_HZ) / 1000;
    if (ms > 0 && target == ticks) target++;  // At least 1 tick
    while (ticks < target) {
        __asm__ volatile("hlt");
    }
}
