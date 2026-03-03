; Kernel entry point - must be linked first to be at 0x10000
[bits 64]

global _start
extern kmain

section .text.entry

_start:
    ; Set up stack
    mov rsp, 0x90000

    ; Call the C kernel main
    call kmain

    ; If kmain returns, halt
.hang:
    cli
    hlt
    jmp .hang
