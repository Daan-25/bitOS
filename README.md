# bitOS

A custom 64-bit operating system built from scratch — including the bootloader. Written in x86 Assembly and C, targeting x86_64.

```
  ____  _ _    ___  ____
 | __ )(_) |_ / _ \/ ___|
 |  _ \| | __| | | \___ \
 | |_) | | |_| |_| |___) |
 |____/|_|\__|\___/|____/
```

## Features

- **Custom two-stage bootloader** — no GRUB, no UEFI, pure BIOS
  - Stage 1: 512-byte MBR, loads stage 2 via BIOS `INT 0x13`
  - Stage 2: GDT setup, protected mode, page tables, long mode switch
- **64-bit C kernel** running in long mode
- **VGA text mode driver** — colored output at `0xB8000`
- **Serial port driver** — COM1 output + input for headless mode
- **Interrupt system** — full IDT with 256 entries, PIC remapping
- **PS/2 keyboard driver** — US QWERTY layout, shift & caps lock
- **Serial input** — type in terminal via `-nographic` mode

## Boot Flow

```
BIOS → Stage 1 (16-bit real mode)
         ↓ loads sectors from disk
       Stage 2 (16-bit → 32-bit protected mode → 64-bit long mode)
         ↓ sets up GDT, page tables, enables paging
       C Kernel (kmain)
         ↓ initializes VGA, serial, IDT, keyboard
       Interactive echo (type and see output)
```

## Building

### Prerequisites (macOS)

```bash
brew install nasm x86_64-elf-gcc x86_64-elf-binutils qemu
```

### Build & Run

```bash
make            # Build disk image
make run        # Run in terminal (serial mode)
make run-gui    # Run with QEMU graphical window (VGA output)
make clean      # Clean build artifacts
```

## Project Structure

```
bitos/
├── Makefile              # Build system
├── linker.ld             # Kernel linker script (loads at 0x10000)
├── boot/
│   ├── stage1.asm        # 512-byte MBR bootloader
│   └── stage2.asm        # GDT, protected mode, long mode switch
├── kernel/
│   ├── entry.asm         # 64-bit kernel entry point (_start → kmain)
│   ├── isr.asm           # ISR/IRQ assembly stubs
│   ├── kernel.c          # kmain — kernel initialization
│   ├── vga.c             # VGA text mode driver
│   ├── serial.c          # COM1 serial port driver
│   ├── idt.c             # Interrupt Descriptor Table + PIC
│   └── keyboard.c        # PS/2 keyboard + serial input driver
└── include/
    ├── types.h           # stdint-style type definitions
    ├── io.h              # Port I/O helpers (inb/outb)
    ├── vga.h             # VGA driver interface
    ├── serial.h          # Serial driver interface
    ├── idt.h             # IDT interface
    └── keyboard.h        # Keyboard driver interface
```

## Disk Image Layout

| Sector(s) | Content            |
|----------|--------------------|
| 0        | Stage 1 (MBR)      |
| 1–4      | Stage 2 bootloader |
| 5+       | Kernel binary      |

## Roadmap

- [x] Custom BIOS bootloader (stage 1 + 2)
- [x] 32-bit protected mode → 64-bit long mode
- [x] VGA text mode + serial output
- [x] IDT, PIC, interrupt handling
- [x] PS/2 keyboard driver
- [ ] Memory management (physical + virtual)
- [ ] Interactive shell with commands
- [ ] Timer (PIT/APIC)
- [ ] Filesystem (FAT)
- [ ] Userspace & syscalls
- [ ] Multitasking

## License

MIT