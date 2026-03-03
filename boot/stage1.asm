; =============================================================================
; bitOS Stage 1 Bootloader
; 512-byte MBR loaded by BIOS at 0x7C00
; - Prints "bitOS" banner
; - Loads stage 2 from disk into memory
; - Jumps to stage 2
; =============================================================================

[bits 16]
[org 0x7C00]

STAGE2_LOAD_ADDR equ 0x7E00      ; Right after the MBR (sector 2+)
STAGE2_SECTORS   equ 4           ; Load 4 sectors (2KB) for stage 2

start:
    ; Set up segments and stack
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00                ; Stack grows down from 0x7C00

    ; Save boot drive number (BIOS passes it in DL)
    mov [boot_drive], dl

    ; Clear screen
    mov ah, 0x00
    mov al, 0x03                  ; 80x25 text mode
    int 0x10

    ; Print banner
    mov si, msg_banner
    call print_string

    ; Load stage 2 from disk
    mov si, msg_loading
    call print_string

    call load_stage2

    ; Jump to stage 2
    mov si, msg_jump
    call print_string

    jmp STAGE2_LOAD_ADDR

; =============================================================================
; load_stage2 - Load sectors from disk using BIOS INT 0x13
; =============================================================================
load_stage2:
    mov ah, 0x02                  ; BIOS read sectors
    mov al, STAGE2_SECTORS        ; Number of sectors to read
    mov ch, 0                     ; Cylinder 0
    mov cl, 2                     ; Start from sector 2 (1-indexed, sector 1 = MBR)
    mov dh, 0                     ; Head 0
    mov dl, [boot_drive]          ; Drive number
    mov bx, STAGE2_LOAD_ADDR     ; Buffer address ES:BX
    int 0x13
    jc disk_error                 ; CF set on error
    ret

disk_error:
    mov si, msg_disk_err
    call print_string
    jmp halt

halt:
    cli
    hlt
    jmp halt

; =============================================================================
; print_string - Print null-terminated string at SI
; =============================================================================
print_string:
    pusha
.loop:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0E                  ; BIOS teletype
    mov bh, 0
    int 0x10
    jmp .loop
.done:
    popa
    ret

; =============================================================================
; Data
; =============================================================================
boot_drive: db 0

msg_banner:
    db '  ____  _ _    ___  ____  ', 13, 10
    db ' | __ )(_) |_ / _ \/ ___| ', 13, 10
    db ' |  _ \| | __| | | \___ \ ', 13, 10
    db " | |_) | | |_| |_| |___) |", 13, 10
    db ' |____/|_|\__|\___/|____/ ', 13, 10
    db 13, 10, 0

msg_loading:  db 'Loading stage 2...', 13, 10, 0
msg_jump:     db 'Jumping to stage 2!', 13, 10, 0
msg_disk_err: db 'Disk read error!', 13, 10, 0

; =============================================================================
; Boot signature - pad to 510 bytes, then 0xAA55
; =============================================================================
times 510 - ($ - $$) db 0
dw 0xAA55
