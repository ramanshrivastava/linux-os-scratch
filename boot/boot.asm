; boot.asm - Minimal Boot Sector
; This is our first boot sector that runs in 16-bit real mode
; BIOS loads this 512-byte sector to memory address 0x7C00 and jumps to it

[BITS 16]           ; Tell assembler we're in 16-bit mode
[ORG 0x7C00]       ; BIOS loads boot sector at address 0x7C00

start:
    ; Setup segment registers (segments are used for memory addressing in real mode)
    xor ax, ax      ; AX = 0 (XOR with itself always gives 0)
    mov ds, ax      ; Data Segment = 0
    mov es, ax      ; Extra Segment = 0
    mov ss, ax      ; Stack Segment = 0
    mov sp, 0x7C00  ; Stack Pointer at 0x7C00 (stack grows downward)
    
    ; Clear screen using BIOS interrupt
    mov ah, 0x00    ; Function 0x00: Set video mode
    mov al, 0x03    ; Mode 0x03: 80x25 color text mode
    int 0x10        ; Call BIOS video interrupt
    
    ; Print our boot message
    mov si, boot_msg    ; SI points to our message string
    call print_string   ; Call our print function
    
    ; Infinite loop to halt execution
    jmp $           ; Jump to current address (infinite loop)

; Function: print_string
; Prints a null-terminated string pointed to by SI register
print_string:
    pusha           ; Save all registers
.loop:
    lodsb           ; Load byte from [SI] into AL, increment SI
    or al, al       ; Check if AL is 0 (end of string)
    jz .done        ; If zero, we're done
    
    ; Print character using BIOS interrupt
    mov ah, 0x0E    ; Function 0x0E: Teletype output
    mov bh, 0x00    ; Page number 0
    mov bl, 0x07    ; Light gray color
    int 0x10        ; Call BIOS video interrupt
    
    jmp .loop       ; Continue with next character
.done:
    popa            ; Restore all registers
    ret             ; Return to caller

; Data section
boot_msg:
    db 'Booting OS...', 0x0D, 0x0A  ; 0x0D = CR, 0x0A = LF (newline)
    db 'Boot sector loaded successfully!', 0x0D, 0x0A
    db 0            ; Null terminator

; Padding and boot signature
; Boot sector must be exactly 512 bytes with 0xAA55 signature at the end
times 510-($-$$) db 0  ; Fill remaining bytes with zeros
dw 0xAA55              ; Boot signature (little-endian)