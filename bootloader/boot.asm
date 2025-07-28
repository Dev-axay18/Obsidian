; 💠 Obsidian OS Bootloader
; x86_64 assembly bootloader for Obsidian OS

[org 0x7c00]
[bits 16]

; Boot sector starts here
start:
    ; Initialize segment registers
    cli                     ; Disable interrupts
    xor ax, ax             ; Clear AX register
    mov ds, ax             ; Set data segment to 0
    mov es, ax             ; Set extra segment to 0
    mov ss, ax             ; Set stack segment to 0
    mov sp, 0x7c00         ; Set stack pointer

    ; Clear screen
    mov ah, 0x00
    mov al, 0x03
    int 0x10

    ; Print welcome message
    mov si, welcome_msg
    call print_string

    ; Load kernel from disk
    mov ah, 0x02           ; BIOS read sector function
    mov al, 15             ; Number of sectors to read (adjust based on kernel size)
    mov ch, 0              ; Cylinder 0
    mov cl, 2              ; Sector 2 (sector 1 is bootloader)
    mov dh, 0              ; Head 0
    mov dl, 0x80           ; Drive 0x80 (first hard disk)
    mov bx, 0x1000         ; Load to memory address 0x1000
    int 0x13               ; BIOS interrupt

    jc disk_error          ; Jump if carry flag is set (error)

    ; Jump to kernel
    jmp 0x1000

disk_error:
    mov si, disk_error_msg
    call print_string
    jmp $                   ; Infinite loop

; Print string function
print_string:
    lodsb                   ; Load byte from SI into AL
    or al, al              ; Check if AL is zero (end of string)
    jz print_done          ; If zero, we're done
    mov ah, 0x0e           ; BIOS teletype function
    int 0x10               ; BIOS interrupt
    jmp print_string       ; Repeat for next character
print_done:
    ret

; Data section
welcome_msg db '💠 Obsidian OS Bootloader v1.0', 0x0d, 0x0a, 'Loading kernel...', 0x0d, 0x0a, 0
disk_error_msg db '❌ Disk read error!', 0x0d, 0x0a, 0

; Boot signature
times 510-($-$$) db 0      ; Fill remaining bytes with zeros
dw 0xaa55                  ; Boot signature 