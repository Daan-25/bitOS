#ifndef IDT_H
#define IDT_H

#include "types.h"

// IDT entry (16 bytes in 64-bit mode)
struct idt_entry {
    uint16_t offset_low;     // Offset bits 0-15
    uint16_t selector;       // Code segment selector
    uint8_t  ist;            // Interrupt Stack Table offset
    uint8_t  type_attr;      // Type and attributes
    uint16_t offset_mid;     // Offset bits 16-31
    uint32_t offset_high;    // Offset bits 32-63
    uint32_t reserved;       // Must be zero
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

// Interrupt handler function type
typedef void (*isr_handler_t)(uint64_t error_code, uint64_t int_no);

void idt_init(void);
void idt_set_gate(uint8_t num, uint64_t handler, uint16_t selector, uint8_t flags);
void isr_register_handler(uint8_t num, isr_handler_t handler);

#endif
