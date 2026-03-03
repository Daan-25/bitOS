ASM      = nasm
CC       = x86_64-elf-gcc
LD       = x86_64-elf-ld
OBJCOPY  = x86_64-elf-objcopy
QEMU     = qemu-system-x86_64

CFLAGS   = -ffreestanding -mno-red-zone -mno-mmx -mno-sse -mno-sse2 \
           -nostdlib -nostdinc -fno-builtin -fno-stack-protector \
           -Wall -Wextra -c -Iinclude
LDFLAGS  = -nostdlib -T linker.ld

BUILD    = build

# Kernel sources
KERNEL_C_SRC = kernel/vga.c kernel/serial.c kernel/string.c kernel/idt.c kernel/keyboard.c \
               kernel/pmm.c kernel/vmm.c kernel/heap.c kernel/shell.c kernel/kernel.c
KERNEL_OBJ   = $(BUILD)/kernel/entry.o $(BUILD)/kernel/isr.o $(patsubst %.c,$(BUILD)/%.o,$(KERNEL_C_SRC))

.PHONY: all clean run run-gui

all: $(BUILD)/bitos.img

# === Stage 1 Bootloader (512-byte MBR) ===
$(BUILD)/stage1.bin: boot/stage1.asm | $(BUILD)
	$(ASM) -f bin $< -o $@

# === Stage 2 Bootloader ===
$(BUILD)/stage2.bin: boot/stage2.asm | $(BUILD)
	$(ASM) -f bin $< -o $@

# === Kernel asm objects ===
$(BUILD)/kernel/entry.o: kernel/entry.asm | $(BUILD)/kernel
	$(ASM) -f elf64 $< -o $@

$(BUILD)/kernel/isr.o: kernel/isr.asm | $(BUILD)/kernel
	$(ASM) -f elf64 $< -o $@

# === Kernel C objects ===
$(BUILD)/kernel/%.o: kernel/%.c | $(BUILD)/kernel
	$(CC) $(CFLAGS) $< -o $@

# === Link kernel ===
$(BUILD)/kernel.elf: $(KERNEL_OBJ)
	$(LD) $(LDFLAGS) $^ -o $@

$(BUILD)/kernel.bin: $(BUILD)/kernel.elf
	$(OBJCOPY) -O binary $< $@

# === Build disk image ===
# Layout: sector 1 = stage1, sectors 2-5 = stage2, sectors 6+ = kernel
$(BUILD)/bitos.img: $(BUILD)/stage1.bin $(BUILD)/stage2.bin $(BUILD)/kernel.bin
	dd if=/dev/zero of=$@ bs=512 count=2880 2>/dev/null
	dd if=$(BUILD)/stage1.bin of=$@ conv=notrunc bs=512 seek=0 2>/dev/null
	dd if=$(BUILD)/stage2.bin of=$@ conv=notrunc bs=512 seek=1 2>/dev/null
	dd if=$(BUILD)/kernel.bin of=$@ conv=notrunc bs=512 seek=5 2>/dev/null

# === Run in QEMU ===
run: $(BUILD)/bitos.img
	$(QEMU) -drive format=raw,file=$< -nographic -monitor none

run-gui: $(BUILD)/bitos.img
	$(QEMU) -drive format=raw,file=$<

# === Directories ===
$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/kernel:
	mkdir -p $(BUILD)/kernel

clean:
	rm -rf $(BUILD)
