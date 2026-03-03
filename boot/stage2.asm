; =============================================================================
; bitOS Stage 2 Bootloader
; Loaded at 0x7E00 by stage 1
; - Sets up GDT
; - Switches to 32-bit protected mode
; - Sets up page tables for long mode
; - Switches to 64-bit long mode
; - Jumps to C kernel
; =============================================================================

[bits 16]
[org 0x7E00]

KERNEL_LOAD_ADDR equ 0x10000     ; Load kernel at 64KB mark
KERNEL_SECTORS   equ 64          ; Load 64 sectors (32KB) for kernel

stage2_start:
    ; Print message in real mode
    mov si, msg_stage2
    call print_string_rm

    ; Load kernel from disk before leaving real mode (need BIOS INT 0x13)
    call load_kernel

    mov si, msg_kernel_loaded
    call print_string_rm

    ; Disable interrupts for mode switch
    cli

    ; Enable A20 line
    call enable_a20

    ; Load GDT
    lgdt [gdt_descriptor]

    ; Set PE bit in CR0 to enter protected mode
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ; Far jump to flush pipeline and load CS with 32-bit code segment
    jmp 0x08:protected_mode_entry

; =============================================================================
; load_kernel - Load kernel sectors from disk using BIOS INT 0x13
; =============================================================================
load_kernel:
    ; Load kernel in two batches (BIOS INT 0x13 can be limited per call)
    ; Batch 1: 32 sectors at sector 6 -> 0x1000:0x0000 (= 0x10000)
    mov ah, 0x02
    mov al, 32
    mov ch, 0
    mov cl, 6                     ; Start sector
    mov dh, 0
    mov dl, 0x80
    mov bx, 0x1000
    mov es, bx
    xor bx, bx
    int 0x13
    jc .disk_error

    ; Batch 2: 32 sectors at sector 38 -> 0x1400:0x0000 (= 0x14000)
    mov ah, 0x02
    mov al, 32
    mov ch, 0
    mov cl, 38                    ; sector 6 + 32
    mov dh, 0
    mov dl, 0x80
    mov bx, 0x1400
    mov es, bx
    xor bx, bx
    int 0x13
    jc .disk_error
    
    ; Reset ES
    xor ax, ax
    mov es, ax
    ret

.disk_error:
    mov si, msg_disk_err
    call print_string_rm
    cli
    hlt

; =============================================================================
; enable_a20 - Enable A20 line via keyboard controller
; =============================================================================
enable_a20:
    ; Try fast A20 gate first (port 0x92)
    in al, 0x92
    or al, 2
    out 0x92, al
    ret

; =============================================================================
; print_string_rm - 16-bit real mode print
; =============================================================================
print_string_rm:
    pusha
.loop:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0E
    mov bh, 0
    int 0x10
    jmp .loop
.done:
    popa
    ret

; =============================================================================
; 32-bit Protected Mode
; =============================================================================
[bits 32]

protected_mode_entry:
    ; Set up segment registers for protected mode
    mov ax, 0x10                  ; Data segment selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000              ; Stack at 576KB

    ; Print to VGA to show we're in protected mode
    mov esi, msg_pm
    mov edi, 0xB8000 + (160 * 10) ; Row 10 on screen
    mov ah, 0x0A                  ; Light green on black
.print_pm:
    lodsb
    or al, al
    jz .done_pm
    mov [edi], ax
    add edi, 2
    jmp .print_pm
.done_pm:

    ; === Set up page tables for long mode ===
    ; We need: PML4 -> PDPT -> PD (2MB pages identity mapping)

    ; Clear page table area (0x1000 - 0x4FFF)
    mov edi, 0x1000
    mov ecx, 0x1000               ; 4096 dwords = 16KB
    xor eax, eax
    rep stosd

    ; PML4[0] -> PDPT at 0x2000
    mov dword [0x1000], 0x2003    ; Present + R/W

    ; PDPT[0] -> PD at 0x3000
    mov dword [0x2000], 0x3003    ; Present + R/W

    ; PD: Map first 1GB using 2MB pages (512 entries)
    mov edi, 0x3000
    mov eax, 0x0000_0083          ; Present + R/W + Page Size (2MB)
    mov ecx, 512
.fill_pd:
    mov [edi], eax
    add eax, 0x200000            ; Next 2MB page
    add edi, 8
    loop .fill_pd

    ; Load PML4 into CR3
    mov eax, 0x1000
    mov cr3, eax

    ; Enable PAE (CR4.PAE = bit 5)
    mov eax, cr4
    or eax, (1 << 5)
    mov cr4, eax

    ; Enable Long Mode (set EFER.LME = bit 8)
    mov ecx, 0xC0000080           ; IA32_EFER MSR
    rdmsr
    or eax, (1 << 8)
    wrmsr

    ; Enable paging (CR0.PG = bit 31)
    mov eax, cr0
    or eax, (1 << 31)
    mov cr0, eax

    ; Load 64-bit GDT and far jump to long mode
    lgdt [gdt64_descriptor]
    jmp 0x08:long_mode_entry

; =============================================================================
; 64-bit Long Mode
; =============================================================================
[bits 64]

long_mode_entry:
    ; Set up segment registers
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Set up stack
    mov rsp, 0x90000

    ; Print to VGA to show we're in long mode
    mov rsi, msg_lm
    mov rdi, 0xB8000 + (160 * 11) ; Row 11
    mov ah, 0x0E                  ; Yellow on black
.print_lm:
    lodsb
    or al, al
    jz .done_lm
    mov [rdi], ax
    add rdi, 2
    jmp .print_lm
.done_lm:

    ; Jump to C kernel at 0x10000
    mov rax, 0x10000
    jmp rax

    ; Should never reach here
    cli
    hlt

; =============================================================================
; GDT for Protected Mode (32-bit)
; =============================================================================
align 16
gdt_start:
    ; Null descriptor
    dq 0

    ; Code segment: base=0, limit=4GB, 32-bit, executable, readable
    dw 0xFFFF                     ; Limit low
    dw 0x0000                     ; Base low
    db 0x00                       ; Base middle
    db 10011010b                  ; Access: Present, Ring0, Code, Exec+Read
    db 11001111b                  ; Flags: 4KB granularity, 32-bit + Limit high
    db 0x00                       ; Base high

    ; Data segment: base=0, limit=4GB, 32-bit, writable
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b                  ; Access: Present, Ring0, Data, Read+Write
    db 11001111b
    db 0x00
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1   ; Size
    dd gdt_start                  ; Offset

; =============================================================================
; GDT for Long Mode (64-bit)
; =============================================================================
align 16
gdt64_start:
    ; Null descriptor
    dq 0

    ; 64-bit Code segment
    dw 0xFFFF                     ; Limit low (ignored in long mode)
    dw 0x0000                     ; Base low
    db 0x00                       ; Base middle
    db 10011010b                  ; Access: Present, Ring0, Code, Exec+Read
    db 10101111b                  ; Flags: Long mode + Limit high
    db 0x00                       ; Base high

    ; 64-bit Data segment
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b                  ; Access: Present, Ring0, Data, Read+Write
    db 10101111b                  ; Long mode flag
    db 0x00
gdt64_end:

gdt64_descriptor:
    dw gdt64_end - gdt64_start - 1
    dd gdt64_start

; =============================================================================
; Data
; =============================================================================
msg_stage2:       db '[Stage 2] Setting up for long mode...', 13, 10, 0
msg_kernel_loaded: db '[Stage 2] Kernel loaded from disk.', 13, 10, 0
msg_disk_err:     db '[Stage 2] Disk error!', 13, 10, 0
msg_pm:           db '[32-bit] Protected mode OK', 0
msg_lm:           db '[64-bit] Long mode OK - jumping to kernel...', 0
