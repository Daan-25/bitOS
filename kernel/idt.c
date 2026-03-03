#include "idt.h"
#include "io.h"
#include "vga.h"
#include "serial.h"

#define IDT_ENTRIES 256
#define PIC1_CMD    0x20
#define PIC1_DATA   0x21
#define PIC2_CMD    0xA0
#define PIC2_DATA   0xA1

static struct idt_entry idt[IDT_ENTRIES];
static struct idt_ptr   idtp;
static isr_handler_t    handlers[IDT_ENTRIES];

// External ISR stubs from isr.asm
extern void isr0(void);  extern void isr1(void);  extern void isr2(void);
extern void isr3(void);  extern void isr4(void);  extern void isr5(void);
extern void isr6(void);  extern void isr7(void);  extern void isr8(void);
extern void isr9(void);  extern void isr10(void); extern void isr11(void);
extern void isr12(void); extern void isr13(void); extern void isr14(void);
extern void isr15(void); extern void isr16(void); extern void isr17(void);
extern void isr18(void); extern void isr19(void); extern void isr20(void);
extern void isr21(void); extern void isr22(void); extern void isr23(void);
extern void isr24(void); extern void isr25(void); extern void isr26(void);
extern void isr27(void); extern void isr28(void); extern void isr29(void);
extern void isr30(void); extern void isr31(void);

extern void irq0(void);  extern void irq1(void);  extern void irq2(void);
extern void irq3(void);  extern void irq4(void);  extern void irq5(void);
extern void irq6(void);  extern void irq7(void);  extern void irq8(void);
extern void irq9(void);  extern void irq10(void); extern void irq11(void);
extern void irq12(void); extern void irq13(void); extern void irq14(void);
extern void irq15(void);

static const char *exception_names[] = {
    "Division By Zero", "Debug", "Non Maskable Interrupt", "Breakpoint",
    "Overflow", "Bound Range Exceeded", "Invalid Opcode", "Device Not Available",
    "Double Fault", "Coprocessor Segment Overrun", "Invalid TSS",
    "Segment Not Present", "Stack-Segment Fault", "General Protection Fault",
    "Page Fault", "Reserved", "x87 FPU Error", "Alignment Check",
    "Machine Check", "SIMD Floating-Point", "Virtualization", "Control Protection",
    "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved"
};

static void pic_remap(void) {
    uint8_t mask1 = inb(PIC1_DATA);
    uint8_t mask2 = inb(PIC2_DATA);

    // ICW1: Start initialization in cascade mode
    outb(PIC1_CMD, 0x11); io_wait();
    outb(PIC2_CMD, 0x11); io_wait();

    // ICW2: Vector offsets (IRQ 0-7 -> ISR 32-39, IRQ 8-15 -> ISR 40-47)
    outb(PIC1_DATA, 0x20); io_wait();
    outb(PIC2_DATA, 0x28); io_wait();

    // ICW3: Tell Master PIC there is a slave at IRQ2
    outb(PIC1_DATA, 0x04); io_wait();
    outb(PIC2_DATA, 0x02); io_wait();

    // ICW4: 8086 mode
    outb(PIC1_DATA, 0x01); io_wait();
    outb(PIC2_DATA, 0x01); io_wait();

    // Restore masks
    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);
}

void idt_set_gate(uint8_t num, uint64_t handler, uint16_t selector, uint8_t flags) {
    idt[num].offset_low  = handler & 0xFFFF;
    idt[num].offset_mid  = (handler >> 16) & 0xFFFF;
    idt[num].offset_high = (handler >> 32) & 0xFFFFFFFF;
    idt[num].selector    = selector;
    idt[num].ist         = 0;
    idt[num].type_attr   = flags;
    idt[num].reserved    = 0;
}

void isr_register_handler(uint8_t num, isr_handler_t handler) {
    handlers[num] = handler;
}

// Called from isr_common_stub in isr.asm
void isr_common_handler(uint64_t int_no, uint64_t error_code) {
    if (handlers[int_no]) {
        handlers[int_no](error_code, int_no);
    } else if (int_no < 32) {
        // Unhandled CPU exception
        vga_set_color(VGA_WHITE, VGA_RED);
        vga_print("\n!!! EXCEPTION: ");
        vga_print(exception_names[int_no]);
        vga_print(" !!!\n");
        serial_print("\n!!! EXCEPTION: ");
        serial_print(exception_names[int_no]);
        serial_print(" !!!\n");
        for (;;) __asm__ volatile("cli; hlt");
    }

    // Send EOI for hardware interrupts (IRQ 0-15 = ISR 32-47)
    if (int_no >= 32 && int_no < 48) {
        if (int_no >= 40) {
            outb(PIC2_CMD, 0x20);  // EOI to slave PIC
        }
        outb(PIC1_CMD, 0x20);     // EOI to master PIC
    }
}

void idt_init(void) {
    // Clear handlers
    for (int i = 0; i < IDT_ENTRIES; i++) {
        handlers[i] = NULL;
    }

    // Remap PIC: IRQs 0-15 to ISR 32-47
    pic_remap();

    // 0x8E = Present, Ring 0, 64-bit Interrupt Gate
    uint8_t flags = 0x8E;
    uint16_t cs = 0x08;

    // CPU Exceptions (ISR 0-31)
    idt_set_gate(0,  (uint64_t)isr0,  cs, flags);
    idt_set_gate(1,  (uint64_t)isr1,  cs, flags);
    idt_set_gate(2,  (uint64_t)isr2,  cs, flags);
    idt_set_gate(3,  (uint64_t)isr3,  cs, flags);
    idt_set_gate(4,  (uint64_t)isr4,  cs, flags);
    idt_set_gate(5,  (uint64_t)isr5,  cs, flags);
    idt_set_gate(6,  (uint64_t)isr6,  cs, flags);
    idt_set_gate(7,  (uint64_t)isr7,  cs, flags);
    idt_set_gate(8,  (uint64_t)isr8,  cs, flags);
    idt_set_gate(9,  (uint64_t)isr9,  cs, flags);
    idt_set_gate(10, (uint64_t)isr10, cs, flags);
    idt_set_gate(11, (uint64_t)isr11, cs, flags);
    idt_set_gate(12, (uint64_t)isr12, cs, flags);
    idt_set_gate(13, (uint64_t)isr13, cs, flags);
    idt_set_gate(14, (uint64_t)isr14, cs, flags);
    idt_set_gate(15, (uint64_t)isr15, cs, flags);
    idt_set_gate(16, (uint64_t)isr16, cs, flags);
    idt_set_gate(17, (uint64_t)isr17, cs, flags);
    idt_set_gate(18, (uint64_t)isr18, cs, flags);
    idt_set_gate(19, (uint64_t)isr19, cs, flags);
    idt_set_gate(20, (uint64_t)isr20, cs, flags);
    idt_set_gate(21, (uint64_t)isr21, cs, flags);
    idt_set_gate(22, (uint64_t)isr22, cs, flags);
    idt_set_gate(23, (uint64_t)isr23, cs, flags);
    idt_set_gate(24, (uint64_t)isr24, cs, flags);
    idt_set_gate(25, (uint64_t)isr25, cs, flags);
    idt_set_gate(26, (uint64_t)isr26, cs, flags);
    idt_set_gate(27, (uint64_t)isr27, cs, flags);
    idt_set_gate(28, (uint64_t)isr28, cs, flags);
    idt_set_gate(29, (uint64_t)isr29, cs, flags);
    idt_set_gate(30, (uint64_t)isr30, cs, flags);
    idt_set_gate(31, (uint64_t)isr31, cs, flags);

    // Hardware IRQs (ISR 32-47)
    idt_set_gate(32, (uint64_t)irq0,  cs, flags);
    idt_set_gate(33, (uint64_t)irq1,  cs, flags);
    idt_set_gate(34, (uint64_t)irq2,  cs, flags);
    idt_set_gate(35, (uint64_t)irq3,  cs, flags);
    idt_set_gate(36, (uint64_t)irq4,  cs, flags);
    idt_set_gate(37, (uint64_t)irq5,  cs, flags);
    idt_set_gate(38, (uint64_t)irq6,  cs, flags);
    idt_set_gate(39, (uint64_t)irq7,  cs, flags);
    idt_set_gate(40, (uint64_t)irq8,  cs, flags);
    idt_set_gate(41, (uint64_t)irq9,  cs, flags);
    idt_set_gate(42, (uint64_t)irq10, cs, flags);
    idt_set_gate(43, (uint64_t)irq11, cs, flags);
    idt_set_gate(44, (uint64_t)irq12, cs, flags);
    idt_set_gate(45, (uint64_t)irq13, cs, flags);
    idt_set_gate(46, (uint64_t)irq14, cs, flags);
    idt_set_gate(47, (uint64_t)irq15, cs, flags);

    // Mask all IRQs except keyboard (IRQ1), cascade (IRQ2), COM1 (IRQ4)
    outb(PIC1_DATA, 0xE9);  // 11101001 - IRQ1, IRQ2, IRQ4 unmasked
    outb(PIC2_DATA, 0xFF);  // All slave IRQs masked

    // Load IDT
    idtp.limit = sizeof(idt) - 1;
    idtp.base  = (uint64_t)&idt;
    __asm__ volatile("lidt %0" : : "m"(idtp));

    // Enable interrupts
    __asm__ volatile("sti");
}
