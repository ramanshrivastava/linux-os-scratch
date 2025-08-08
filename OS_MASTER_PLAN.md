# Operating System from Scratch - Master Plan

## Overview
Building a complete operating system from scratch using Assembly (x86_64) and C. This OS will boot on real hardware or QEMU, providing complete visibility into every component.

## Why Assembly and C?

### Why Not C++?
- **Hidden complexity** - Constructors/destructors run automatically (problematic before memory management exists)
- **Runtime dependencies** - Needs runtime support for exceptions, RTTI, new/delete
- **Name mangling** - Makes assembly interfacing complex
- **Unpredictable overhead** - Virtual functions, templates can generate unexpected code
- **No standard library** - Can't use STL, iostream without OS support

### How Assembly Works
Assembly is the human-readable version of machine code:
- **Direct CPU instructions**: `mov rax, 5` puts value 5 in register RAX
- **Registers**: CPU's fast temporary storage (RAX, RBX, RCX, etc.)
- **Memory addressing**: Direct control of RAM access
- **Stack operations**: PUSH/POP for function calls
- **Hardware control**: IN/OUT instructions for ports

Example:
```asm
mov ax, 0x07E0    ; Put value in AX register
mov ds, ax        ; Set data segment
jmp start         ; Jump to 'start' label
```

## Development Environment Setup

### For macOS:
```bash
# Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install required tools
brew install nasm                  # Netwide Assembler for x86 assembly
brew install qemu                  # Emulator to test our OS
brew install x86_64-elf-gcc        # Cross-compiler for x86_64
brew install x86_64-elf-binutils   # Linker, assembler for x86_64
brew install make                  # Build automation
```

**Why no virtual environment?**
- Assembly/C compile to native machine code
- No package dependencies like Python
- Tools are system-wide compilers/assemblers
- Our OS runs on bare metal (or emulated hardware)

## Project Structure
```
/boot/
  boot.asm          # Stage 1 bootloader (512 bytes)
  stage2.asm        # Stage 2 bootloader
/kernel/
  entry.asm         # Kernel entry point
  kernel.c          # Main kernel code
  interrupt.asm     # Interrupt handlers
  process.asm       # Context switching
/drivers/
  vga.c            # Screen driver
  keyboard.c       # Keyboard driver
  timer.c          # Timer driver
/mm/
  pmm.c            # Physical memory manager
  vmm.c            # Virtual memory manager
  heap.c           # Heap allocator
/include/
  system.h         # System definitions
  types.h          # Type definitions
/build/
  Makefile         # Build configuration
  linker.ld        # Linker script
```

---

## PHASE 1: Boot Sector - Making Computer Start (Day 1-2)

### Understanding Boot Process
1. CPU starts in 16-bit real mode at address 0xFFFF0
2. BIOS runs Power-On Self Test (POST)
3. BIOS loads first 512 bytes from disk to RAM at 0x7C00
4. If last 2 bytes are 0xAA55 (boot signature), BIOS jumps to 0x7C00
5. Our code takes control!

### First Boot Sector (`boot/boot.asm`)
```asm
; This runs in 16-bit real mode
[BITS 16]           ; Tell assembler we're in 16-bit mode
[ORG 0x7C00]       ; BIOS loads us here

start:
    ; Setup segments (think of these as memory regions)
    xor ax, ax      ; AX = 0
    mov ds, ax      ; Data Segment = 0
    mov es, ax      ; Extra Segment = 0
    mov ss, ax      ; Stack Segment = 0
    mov sp, 0x7C00  ; Stack grows downward from where we're loaded

    ; Clear screen by calling BIOS interrupt
    mov ah, 0x00    ; Function: Set video mode
    mov al, 0x03    ; Mode: 80x25 color text
    int 0x10        ; Call BIOS video interrupt

    ; Print "Booting OS..." using BIOS
    mov si, boot_msg
    call print_string

    ; Load Stage 2 bootloader from disk
    call load_stage2
    
    ; Jump to Stage 2
    jmp 0x0000:0x7E00

; Function to print string
print_string:
    lodsb           ; Load byte from [SI] into AL
    or al, al       ; Check if 0 (string end)
    jz .done
    mov ah, 0x0E    ; BIOS teletype function
    int 0x10        ; Print character
    jmp print_string
.done:
    ret

; Function to load Stage 2 from disk
load_stage2:
    mov ah, 0x02    ; BIOS read sectors function
    mov al, 10      ; Read 10 sectors
    mov ch, 0       ; Cylinder 0
    mov cl, 2       ; Start from sector 2
    mov dh, 0       ; Head 0
    mov dl, 0x80    ; First hard disk
    mov bx, 0x7E00  ; Load to address 0x7E00
    int 0x13        ; BIOS disk interrupt
    ret

boot_msg db "Booting our OS...", 0x0D, 0x0A, 0

; Boot sector must be exactly 512 bytes with signature
times 510-($-$$) db 0  ; Pad with zeros
dw 0xAA55              ; Boot signature
```

---

## PHASE 2: Stage 2 Bootloader - Entering 32-bit Mode (Day 3-4)

### Why Protected Mode?
- **Real Mode**: 16-bit, can only access 1MB RAM, no memory protection
- **Protected Mode**: 32-bit, access 4GB RAM, memory protection, multitasking support

### Global Descriptor Table (GDT) Setup (`boot/stage2.asm`)
```asm
[BITS 16]
[ORG 0x7E00]

stage2_start:
    ; Disable interrupts during mode switch
    cli
    
    ; Enable A20 line (allows access to memory above 1MB)
    call enable_a20
    
    ; Load GDT
    lgdt [gdt_descriptor]
    
    ; Switch to protected mode by setting bit 0 of CR0
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    
    ; Far jump to 32-bit code
    jmp 0x08:protected_mode_start

; A20 line enabling (multiple methods for compatibility)
enable_a20:
    ; Method 1: Keyboard controller
    call wait_kbd_in
    mov al, 0xAD        ; Disable keyboard
    out 0x64, al
    
    call wait_kbd_in
    mov al, 0xD0        ; Read output port
    out 0x64, al
    
    call wait_kbd_out
    in al, 0x60         ; Read current state
    push ax
    
    call wait_kbd_in
    mov al, 0xD1        ; Write output port
    out 0x64, al
    
    call wait_kbd_in
    pop ax
    or al, 2            ; Set A20 bit
    out 0x60, al
    
    call wait_kbd_in
    mov al, 0xAE        ; Enable keyboard
    out 0x64, al
    ret

wait_kbd_in:
    in al, 0x64
    test al, 2
    jnz wait_kbd_in
    ret

wait_kbd_out:
    in al, 0x64
    test al, 1
    jz wait_kbd_out
    ret

; GDT (Global Descriptor Table) - Defines memory segments
gdt_start:
gdt_null:           ; Null descriptor (required)
    dq 0

gdt_code:           ; Code segment descriptor
    dw 0xFFFF       ; Limit (low)
    dw 0            ; Base (low)
    db 0            ; Base (middle)
    db 10011010b    ; Access byte
    db 11001111b    ; Flags + Limit (high)
    db 0            ; Base (high)

gdt_data:           ; Data segment descriptor
    dw 0xFFFF
    dw 0
    db 0
    db 10010010b
    db 11001111b
    db 0

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1  ; Size
    dd gdt_start                ; Address

[BITS 32]  ; Now we're in 32-bit mode!
protected_mode_start:
    ; Setup segment registers
    mov ax, 0x10    ; Data segment selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Setup stack
    mov esp, 0x90000
    
    ; Load kernel from disk (using ATA PIO mode)
    call load_kernel
    
    ; Jump to kernel
    jmp 0x100000
```

---

## PHASE 3: 64-bit Long Mode & Kernel Entry (Day 5-6)

### Switching to 64-bit Mode (`boot/long_mode.asm`)
```asm
; Check CPU supports 64-bit
check_long_mode:
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .no_long_mode
    
    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29  ; Test LM bit
    jz .no_long_mode
    ret

.no_long_mode:
    ; Display error and halt
    mov esi, no_lm_msg
    call print_string_32
    hlt

; Setup paging for 64-bit mode
setup_paging:
    ; Clear page tables
    mov edi, 0x1000
    mov cr3, edi
    xor eax, eax
    mov ecx, 4096
    rep stosd
    
    ; Setup page tables (identity mapping first 2MB)
    mov edi, 0x1000
    mov dword [edi], 0x2003      ; PML4[0] -> PDPT
    add edi, 0x1000
    mov dword [edi], 0x3003      ; PDPT[0] -> PDT
    add edi, 0x1000
    mov dword [edi], 0x4003      ; PDT[0] -> PT
    add edi, 0x1000
    
    ; Identity map first 2MB
    mov ebx, 0x00000003
    mov ecx, 512
.set_entry:
    mov dword [edi], ebx
    add ebx, 0x1000
    add edi, 8
    loop .set_entry
    
    ; Enable PAE
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax
    
    ; Set long mode bit
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr
    
    ; Enable paging
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax
    ret
```

### Kernel Entry Point (`kernel/entry.asm`)
```asm
[BITS 64]
section .text
global kernel_entry
extern kernel_main

kernel_entry:
    ; Clear segment registers
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Setup stack
    mov rsp, stack_top
    
    ; Clear BSS section (uninitialized data)
    extern __bss_start
    extern __bss_end
    mov rdi, __bss_start
    mov rcx, __bss_end
    sub rcx, rdi
    xor rax, rax
    rep stosb
    
    ; Call C kernel
    call kernel_main
    
    ; If kernel returns, halt
.halt:
    cli
    hlt
    jmp .halt

section .bss
align 16
stack_bottom:
    resb 16384  ; 16KB stack
stack_top:
```

---

## PHASE 4: C Kernel - Finally Writing in C! (Day 7-8)

### Main Kernel (`kernel/kernel.c`)
```c
#include "system.h"
#include "vga.h"
#include "gdt.h"
#include "idt.h"
#include "timer.h"
#include "keyboard.h"

void kernel_main(void) {
    // Initialize VGA text mode
    vga_init();
    vga_print("Welcome to our OS!\n");
    vga_print("Kernel loaded successfully at 0x100000\n\n");
    
    // Initialize core systems
    vga_print("Initializing GDT... ");
    gdt_init();
    vga_print("OK\n");
    
    vga_print("Initializing IDT... ");
    idt_init();
    vga_print("OK\n");
    
    vga_print("Initializing Timer... ");
    timer_init(100);  // 100Hz
    vga_print("OK\n");
    
    vga_print("Initializing Keyboard... ");
    keyboard_init();
    vga_print("OK\n");
    
    vga_print("\nSystem ready!\n> ");
    
    // Main kernel loop
    while(1) {
        // Handle events, will be expanded later
        asm volatile("hlt");
    }
}
```

### VGA Driver (`drivers/vga.c`)
```c
#include "vga.h"
#include "io.h"

#define VGA_MEMORY 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

static uint16_t* vga_buffer = (uint16_t*)VGA_MEMORY;
static uint8_t cursor_x = 0;
static uint8_t cursor_y = 0;
static uint8_t vga_color = 0x07;  // White on black

void vga_init(void) {
    vga_clear();
}

void vga_clear(void) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = (vga_color << 8) | ' ';
    }
    cursor_x = 0;
    cursor_y = 0;
    vga_update_cursor();
}

void vga_putchar(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\r') {
        cursor_x = 0;
    } else if (c == '\b') {
        if (cursor_x > 0) cursor_x--;
    } else if (c == '\t') {
        cursor_x = (cursor_x + 8) & ~7;
    } else {
        vga_buffer[cursor_y * VGA_WIDTH + cursor_x] = (vga_color << 8) | c;
        cursor_x++;
    }
    
    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }
    
    if (cursor_y >= VGA_HEIGHT) {
        vga_scroll();
        cursor_y = VGA_HEIGHT - 1;
    }
    
    vga_update_cursor();
}

void vga_print(const char* str) {
    while (*str) {
        vga_putchar(*str++);
    }
}

void vga_update_cursor(void) {
    uint16_t pos = cursor_y * VGA_WIDTH + cursor_x;
    
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void vga_scroll(void) {
    // Move all lines up by one
    for (int i = 0; i < (VGA_HEIGHT - 1) * VGA_WIDTH; i++) {
        vga_buffer[i] = vga_buffer[i + VGA_WIDTH];
    }
    
    // Clear last line
    for (int i = (VGA_HEIGHT - 1) * VGA_WIDTH; i < VGA_HEIGHT * VGA_WIDTH; i++) {
        vga_buffer[i] = (vga_color << 8) | ' ';
    }
}
```

---

## PHASE 5: Interrupt Handling System (Day 9-10)

### Interrupt Descriptor Table (`kernel/idt.c`)
```c
#include "idt.h"
#include "io.h"
#include "vga.h"

// IDT entry structure
struct idt_entry {
    uint16_t base_low;
    uint16_t selector;
    uint8_t  always0;
    uint8_t  flags;
    uint16_t base_middle;
    uint32_t base_high;
    uint32_t reserved;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

static struct idt_entry idt[256];
static struct idt_ptr idtp;

// Exception messages
static const char* exception_messages[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 FPU Error",
    // ... continue for all 32 exceptions
};

void idt_set_gate(uint8_t num, uint64_t base, uint16_t selector, uint8_t flags) {
    idt[num].base_low = base & 0xFFFF;
    idt[num].base_middle = (base >> 16) & 0xFFFF;
    idt[num].base_high = (base >> 32) & 0xFFFFFFFF;
    idt[num].selector = selector;
    idt[num].always0 = 0;
    idt[num].flags = flags;
    idt[num].reserved = 0;
}

// Common interrupt handler
void interrupt_handler(struct interrupt_frame* frame) {
    if (frame->int_no < 32) {
        // CPU exception
        vga_print("\nEXCEPTION: ");
        vga_print(exception_messages[frame->int_no]);
        vga_print("\n");
        
        if (frame->int_no == 14) {  // Page fault
            uint64_t faulting_address;
            asm volatile("mov %%cr2, %0" : "=r" (faulting_address));
            vga_print("Page fault at: 0x");
            vga_print_hex(faulting_address);
            vga_print("\n");
        }
        
        // Halt on critical exceptions
        while(1) { asm volatile("hlt"); }
    } else if (frame->int_no >= 32 && frame->int_no < 48) {
        // Hardware interrupt (IRQ)
        if (frame->int_no == 32) {
            // Timer interrupt
            timer_handler();
        } else if (frame->int_no == 33) {
            // Keyboard interrupt
            keyboard_handler();
        }
        
        // Send EOI to PIC
        if (frame->int_no >= 40) {
            outb(0xA0, 0x20);  // Slave PIC
        }
        outb(0x20, 0x20);  // Master PIC
    }
}

void idt_init(void) {
    // Remap PIC
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);  // IRQ0-7 -> INT 32-39
    outb(0xA1, 0x28);  // IRQ8-15 -> INT 40-47
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0x00);
    outb(0xA1, 0x00);
    
    // Setup IDT entries
    for (int i = 0; i < 48; i++) {
        idt_set_gate(i, (uint64_t)isr_stub_table[i], 0x08, 0x8E);
    }
    
    // Load IDT
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
    idtp.base = (uint64_t)&idt;
    asm volatile("lidt %0" : : "m" (idtp));
    asm volatile("sti");  // Enable interrupts
}
```

---

## PHASE 6: Memory Management (Day 11-13)

### Physical Memory Manager (`mm/pmm.c`)
```c
#include "pmm.h"
#include "string.h"

#define PAGE_SIZE 4096
#define PAGES_PER_BITMAP 32

static uint32_t* bitmap;
static uint32_t total_pages;
static uint32_t used_pages;

void pmm_init(uint64_t mem_size) {
    total_pages = mem_size / PAGE_SIZE;
    used_pages = 0;
    
    // Place bitmap after kernel
    extern uint32_t __kernel_end;
    bitmap = (uint32_t*)&__kernel_end;
    
    // Calculate bitmap size
    uint32_t bitmap_size = total_pages / 32;
    
    // Mark all pages as used initially
    memset(bitmap, 0xFF, bitmap_size * sizeof(uint32_t));
    
    // Mark available RAM as free (skip first 1MB and kernel)
    uint32_t kernel_pages = ((uint32_t)&__kernel_end - 0x100000) / PAGE_SIZE;
    for (uint32_t i = 256 + kernel_pages; i < total_pages; i++) {
        pmm_free_page(i * PAGE_SIZE);
    }
}

void* pmm_alloc_page(void) {
    for (uint32_t i = 0; i < total_pages / 32; i++) {
        if (bitmap[i] != 0xFFFFFFFF) {
            for (int j = 0; j < 32; j++) {
                if (!(bitmap[i] & (1 << j))) {
                    bitmap[i] |= (1 << j);
                    used_pages++;
                    return (void*)((i * 32 + j) * PAGE_SIZE);
                }
            }
        }
    }
    return NULL;  // Out of memory
}

void pmm_free_page(void* addr) {
    uint32_t page = (uint32_t)addr / PAGE_SIZE;
    bitmap[page / 32] &= ~(1 << (page % 32));
    used_pages--;
}
```

### Virtual Memory Manager (`mm/vmm.c`)
```c
#include "vmm.h"
#include "pmm.h"

// Page table entry flags
#define PAGE_PRESENT  0x001
#define PAGE_WRITE    0x002
#define PAGE_USER     0x004
#define PAGE_SIZE_4MB 0x080

typedef struct {
    uint64_t entries[512];
} page_table_t;

static page_table_t* pml4;

void vmm_init(void) {
    // Allocate PML4
    pml4 = (page_table_t*)pmm_alloc_page();
    memset(pml4, 0, PAGE_SIZE);
    
    // Identity map first 4MB for kernel
    vmm_map_page(0, 0, PAGE_PRESENT | PAGE_WRITE);
    
    // Load new page table
    asm volatile("mov %0, %%cr3" : : "r" (pml4));
}

void vmm_map_page(uint64_t virtual, uint64_t physical, uint64_t flags) {
    // Calculate indices
    uint64_t pml4_idx = (virtual >> 39) & 0x1FF;
    uint64_t pdpt_idx = (virtual >> 30) & 0x1FF;
    uint64_t pd_idx   = (virtual >> 21) & 0x1FF;
    uint64_t pt_idx   = (virtual >> 12) & 0x1FF;
    
    // Get or create PDPT
    page_table_t* pdpt;
    if (!(pml4->entries[pml4_idx] & PAGE_PRESENT)) {
        pdpt = (page_table_t*)pmm_alloc_page();
        memset(pdpt, 0, PAGE_SIZE);
        pml4->entries[pml4_idx] = (uint64_t)pdpt | PAGE_PRESENT | PAGE_WRITE;
    } else {
        pdpt = (page_table_t*)(pml4->entries[pml4_idx] & ~0xFFF);
    }
    
    // Continue for PD and PT...
    // (Similar pattern for page directory and page table)
    
    // Set the final page table entry
    pt->entries[pt_idx] = physical | flags;
    
    // Flush TLB for this address
    asm volatile("invlpg (%0)" : : "r" (virtual));
}
```

### Heap Allocator (`mm/heap.c`)
```c
#include "heap.h"
#include "vmm.h"

typedef struct heap_block {
    size_t size;
    struct heap_block* next;
    struct heap_block* prev;
    uint8_t free;
} heap_block_t;

static heap_block_t* heap_start = NULL;
static uint64_t heap_current = 0x10000000;  // Start heap at 256MB

void heap_init(void) {
    // Allocate initial heap page
    void* page = pmm_alloc_page();
    vmm_map_page(heap_current, (uint64_t)page, PAGE_PRESENT | PAGE_WRITE);
    
    heap_start = (heap_block_t*)heap_current;
    heap_start->size = PAGE_SIZE - sizeof(heap_block_t);
    heap_start->next = NULL;
    heap_start->prev = NULL;
    heap_start->free = 1;
    
    heap_current += PAGE_SIZE;
}

void* malloc(size_t size) {
    // Align size to 8 bytes
    size = (size + 7) & ~7;
    
    heap_block_t* current = heap_start;
    
    while (current) {
        if (current->free && current->size >= size) {
            // Found suitable block
            if (current->size > size + sizeof(heap_block_t) + 8) {
                // Split block
                heap_block_t* new_block = (heap_block_t*)((uint8_t*)current + sizeof(heap_block_t) + size);
                new_block->size = current->size - size - sizeof(heap_block_t);
                new_block->next = current->next;
                new_block->prev = current;
                new_block->free = 1;
                
                if (current->next) {
                    current->next->prev = new_block;
                }
                current->next = new_block;
                current->size = size;
            }
            
            current->free = 0;
            return (void*)((uint8_t*)current + sizeof(heap_block_t));
        }
        current = current->next;
    }
    
    // No suitable block found, expand heap
    void* page = pmm_alloc_page();
    vmm_map_page(heap_current, (uint64_t)page, PAGE_PRESENT | PAGE_WRITE);
    
    // Create new block
    heap_block_t* new_block = (heap_block_t*)heap_current;
    new_block->size = PAGE_SIZE - sizeof(heap_block_t);
    new_block->next = NULL;
    new_block->prev = NULL;
    new_block->free = 1;
    
    // Link to existing heap
    current = heap_start;
    while (current->next) {
        current = current->next;
    }
    current->next = new_block;
    new_block->prev = current;
    
    heap_current += PAGE_SIZE;
    
    // Recursively allocate from new block
    return malloc(size);
}

void free(void* ptr) {
    if (!ptr) return;
    
    heap_block_t* block = (heap_block_t*)((uint8_t*)ptr - sizeof(heap_block_t));
    block->free = 1;
    
    // Coalesce with next block if free
    if (block->next && block->next->free) {
        block->size += sizeof(heap_block_t) + block->next->size;
        block->next = block->next->next;
        if (block->next) {
            block->next->prev = block;
        }
    }
    
    // Coalesce with previous block if free
    if (block->prev && block->prev->free) {
        block->prev->size += sizeof(heap_block_t) + block->size;
        block->prev->next = block->next;
        if (block->next) {
            block->next->prev = block->prev;
        }
    }
}
```

---

## PHASE 7: Process Management & Multitasking (Day 14-16)

### Process Control Block (`kernel/process.h`)
```c
typedef struct process {
    uint32_t pid;
    uint32_t ppid;  // Parent PID
    
    // CPU context
    uint64_t rsp;
    uint64_t rbp;
    uint64_t rip;
    uint64_t rflags;
    uint64_t cr3;  // Page table
    
    // General purpose registers
    uint64_t rax, rbx, rcx, rdx;
    uint64_t rsi, rdi;
    uint64_t r8, r9, r10, r11;
    uint64_t r12, r13, r14, r15;
    
    // Process state
    enum {
        PROCESS_READY,
        PROCESS_RUNNING,
        PROCESS_BLOCKED,
        PROCESS_ZOMBIE
    } state;
    
    // Memory info
    void* stack_base;
    size_t stack_size;
    void* heap_base;
    size_t heap_size;
    
    // Scheduling
    uint32_t priority;
    uint32_t time_slice;
    
    struct process* next;
} process_t;
```

### Scheduler (`kernel/scheduler.c`)
```c
#include "scheduler.h"
#include "process.h"
#include "timer.h"

static process_t* current_process = NULL;
static process_t* ready_queue = NULL;
static uint32_t next_pid = 1;

void scheduler_init(void) {
    // Create kernel process (PID 0)
    process_t* kernel_proc = malloc(sizeof(process_t));
    kernel_proc->pid = 0;
    kernel_proc->ppid = 0;
    kernel_proc->state = PROCESS_RUNNING;
    kernel_proc->priority = 0;
    kernel_proc->time_slice = 10;
    kernel_proc->next = NULL;
    
    current_process = kernel_proc;
}

process_t* create_process(void (*entry_point)(void)) {
    process_t* proc = malloc(sizeof(process_t));
    
    proc->pid = next_pid++;
    proc->ppid = current_process->pid;
    proc->state = PROCESS_READY;
    proc->priority = 5;
    proc->time_slice = 10;
    
    // Allocate stack
    proc->stack_size = 8192;  // 8KB
    proc->stack_base = malloc(proc->stack_size);
    proc->rsp = (uint64_t)proc->stack_base + proc->stack_size;
    proc->rbp = proc->rsp;
    
    // Setup initial context
    proc->rip = (uint64_t)entry_point;
    proc->rflags = 0x202;  // Interrupts enabled
    
    // Allocate page table (copy kernel mappings)
    proc->cr3 = (uint64_t)pmm_alloc_page();
    // ... copy kernel page mappings ...
    
    // Add to ready queue
    proc->next = ready_queue;
    ready_queue = proc;
    
    return proc;
}

void schedule(void) {
    if (!ready_queue) return;
    
    // Save current process context
    if (current_process->state == PROCESS_RUNNING) {
        current_process->state = PROCESS_READY;
        
        // Add back to ready queue
        process_t* tail = ready_queue;
        while (tail->next) tail = tail->next;
        tail->next = current_process;
        current_process->next = NULL;
    }
    
    // Get next process
    process_t* next = ready_queue;
    ready_queue = ready_queue->next;
    
    // Switch context
    switch_context(current_process, next);
    current_process = next;
    current_process->state = PROCESS_RUNNING;
}
```

---

## PHASE 8: System Calls (Day 17-18)

### System Call Interface (`kernel/syscall.c`)
```c
#include "syscall.h"

// System call numbers
#define SYS_EXIT    0
#define SYS_WRITE   1
#define SYS_READ    2
#define SYS_OPEN    3
#define SYS_CLOSE   4
#define SYS_FORK    5
#define SYS_EXEC    6
#define SYS_WAIT    7
#define SYS_MALLOC  8
#define SYS_FREE    9

typedef uint64_t (*syscall_handler_t)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);

static syscall_handler_t syscall_table[256];

// System call implementations
uint64_t sys_exit(int status) {
    current_process->state = PROCESS_ZOMBIE;
    schedule();
    return 0;
}

uint64_t sys_write(int fd, const char* buf, size_t count) {
    if (fd == 1) {  // stdout
        for (size_t i = 0; i < count; i++) {
            vga_putchar(buf[i]);
        }
        return count;
    }
    return -1;
}

uint64_t sys_read(int fd, char* buf, size_t count) {
    if (fd == 0) {  // stdin
        size_t i = 0;
        while (i < count) {
            char c = keyboard_getchar();
            if (c) {
                buf[i++] = c;
                if (c == '\n') break;
            }
        }
        return i;
    }
    return -1;
}

void syscall_init(void) {
    // Register system calls
    syscall_table[SYS_EXIT] = (syscall_handler_t)sys_exit;
    syscall_table[SYS_WRITE] = (syscall_handler_t)sys_write;
    syscall_table[SYS_READ] = (syscall_handler_t)sys_read;
    // ... register other syscalls ...
    
    // Install syscall interrupt handler (INT 0x80)
    idt_set_gate(0x80, (uint64_t)syscall_entry, 0x08, 0xEE);  // User callable
}

// Syscall entry point (called from assembly)
uint64_t syscall_handler(uint64_t nr, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5) {
    if (nr < 256 && syscall_table[nr]) {
        return syscall_table[nr](a1, a2, a3, a4, a5);
    }
    return -1;  // Invalid syscall
}
```

### User Mode Library (`user/syscall.h`)
```c
// User-space syscall wrappers
static inline long syscall(long nr, long a1, long a2, long a3, long a4, long a5) {
    long ret;
    asm volatile(
        "mov %1, %%rax\n"
        "mov %2, %%rdi\n"
        "mov %3, %%rsi\n"
        "mov %4, %%rdx\n"
        "mov %5, %%r10\n"
        "mov %6, %%r8\n"
        "int $0x80\n"
        "mov %%rax, %0"
        : "=r" (ret)
        : "r" (nr), "r" (a1), "r" (a2), "r" (a3), "r" (a4), "r" (a5)
        : "rax", "rdi", "rsi", "rdx", "r10", "r8"
    );
    return ret;
}

#define exit(status)     syscall(0, status, 0, 0, 0, 0)
#define write(fd, buf, count) syscall(1, fd, (long)buf, count, 0, 0)
#define read(fd, buf, count)  syscall(2, fd, (long)buf, count, 0, 0)
```

---

## PHASE 9: Simple Shell (Day 19-20)

### Shell Implementation (`shell/shell.c`)
```c
#include "shell.h"
#include "string.h"
#include "syscall.h"

#define MAX_CMD_LEN 256
#define MAX_ARGS 16

typedef struct {
    const char* name;
    void (*handler)(int argc, char* argv[]);
} command_t;

// Built-in commands
void cmd_help(int argc, char* argv[]) {
    printf("Available commands:\n");
    printf("  help    - Show this help\n");
    printf("  clear   - Clear screen\n");
    printf("  echo    - Print arguments\n");
    printf("  ps      - List processes\n");
    printf("  mem     - Show memory info\n");
    printf("  reboot  - Restart system\n");
}

void cmd_clear(int argc, char* argv[]) {
    vga_clear();
}

void cmd_echo(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        printf("%s ", argv[i]);
    }
    printf("\n");
}

void cmd_ps(int argc, char* argv[]) {
    printf("PID  PPID  STATE     COMMAND\n");
    process_t* proc = get_process_list();
    while (proc) {
        printf("%-4d %-5d %-9s %s\n", 
               proc->pid, proc->ppid,
               state_to_string(proc->state),
               proc->name);
        proc = proc->next;
    }
}

void cmd_mem(int argc, char* argv[]) {
    memory_info_t info = get_memory_info();
    printf("Memory Information:\n");
    printf("  Total:     %d MB\n", info.total / 1024 / 1024);
    printf("  Used:      %d MB\n", info.used / 1024 / 1024);
    printf("  Free:      %d MB\n", info.free / 1024 / 1024);
    printf("  Page Size: %d bytes\n", PAGE_SIZE);
}

static command_t commands[] = {
    {"help", cmd_help},
    {"clear", cmd_clear},
    {"echo", cmd_echo},
    {"ps", cmd_ps},
    {"mem", cmd_mem},
    {NULL, NULL}
};

void shell_main(void) {
    char input[MAX_CMD_LEN];
    char* argv[MAX_ARGS];
    
    printf("\nSimple Shell v1.0\n");
    printf("Type 'help' for available commands\n\n");
    
    while (1) {
        printf("> ");
        
        // Read input
        if (!readline(input, MAX_CMD_LEN)) {
            continue;
        }
        
        // Parse command
        int argc = 0;
        char* token = strtok(input, " \t\n");
        while (token && argc < MAX_ARGS) {
            argv[argc++] = token;
            token = strtok(NULL, " \t\n");
        }
        
        if (argc == 0) continue;
        
        // Find and execute command
        int found = 0;
        for (int i = 0; commands[i].name; i++) {
            if (strcmp(argv[0], commands[i].name) == 0) {
                commands[i].handler(argc, argv);
                found = 1;
                break;
            }
        }
        
        if (!found) {
            printf("Unknown command: %s\n", argv[0]);
        }
    }
}

int readline(char* buf, int max) {
    int i = 0;
    
    while (i < max - 1) {
        char c = getchar();
        
        if (c == '\n') {
            buf[i] = '\0';
            putchar('\n');
            return i;
        } else if (c == '\b') {
            if (i > 0) {
                i--;
                putchar('\b');
                putchar(' ');
                putchar('\b');
            }
        } else if (c >= 32 && c < 127) {
            buf[i++] = c;
            putchar(c);
        }
    }
    
    buf[i] = '\0';
    return i;
}
```

---

## Build System

### Makefile
```makefile
# Toolchain
AS = nasm
CC = x86_64-elf-gcc
LD = x86_64-elf-ld
OBJCOPY = x86_64-elf-objcopy

# Flags
ASFLAGS = -f elf64
CFLAGS = -ffreestanding -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 \
         -nostdlib -nostdinc -fno-builtin -fno-stack-protector -nostartfiles \
         -nodefaultlibs -Wall -Wextra -Werror -c -I include
LDFLAGS = -T linker.ld -nostdlib

# Directories
SRC_DIR = .
BUILD_DIR = build
ISO_DIR = iso

# Source files
BOOT_ASM = $(wildcard boot/*.asm)
KERNEL_ASM = $(wildcard kernel/*.asm)
KERNEL_C = $(wildcard kernel/*.c) $(wildcard drivers/*.c) $(wildcard mm/*.c)

# Object files
BOOT_OBJ = $(BOOT_ASM:boot/%.asm=$(BUILD_DIR)/boot_%.o)
KERNEL_ASM_OBJ = $(KERNEL_ASM:kernel/%.asm=$(BUILD_DIR)/kernel_%.o)
KERNEL_C_OBJ = $(KERNEL_C:%.c=$(BUILD_DIR)/%.o)

# Targets
all: os.iso

# Boot sector
$(BUILD_DIR)/boot.bin: boot/boot.asm
	@mkdir -p $(BUILD_DIR)
	$(AS) -f bin $< -o $@

# Stage 2 bootloader
$(BUILD_DIR)/stage2.bin: boot/stage2.asm
	$(AS) -f bin $< -o $@

# Kernel objects
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.asm
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

# Link kernel
$(BUILD_DIR)/kernel.elf: $(KERNEL_ASM_OBJ) $(KERNEL_C_OBJ)
	$(LD) $(LDFLAGS) $^ -o $@

# Create kernel binary
$(BUILD_DIR)/kernel.bin: $(BUILD_DIR)/kernel.elf
	$(OBJCOPY) -O binary $< $@

# Create disk image
os.img: $(BUILD_DIR)/boot.bin $(BUILD_DIR)/stage2.bin $(BUILD_DIR)/kernel.bin
	dd if=/dev/zero of=$@ bs=1M count=10
	dd if=$(BUILD_DIR)/boot.bin of=$@ conv=notrunc
	dd if=$(BUILD_DIR)/stage2.bin of=$@ seek=1 conv=notrunc
	dd if=$(BUILD_DIR)/kernel.bin of=$@ seek=11 conv=notrunc

# Create ISO (optional, for CD booting)
os.iso: os.img
	@mkdir -p $(ISO_DIR)
	cp os.img $(ISO_DIR)/
	genisoimage -R -b os.img -no-emul-boot -boot-load-size 4 \
	            -o os.iso $(ISO_DIR)

# Run in QEMU
run: os.img
	qemu-system-x86_64 -drive format=raw,file=os.img -m 128M \
	                   -serial stdio -monitor telnet::45454,server,nowait

# Debug in QEMU with GDB
debug: os.img
	qemu-system-x86_64 -drive format=raw,file=os.img -m 128M \
	                   -serial stdio -s -S &
	gdb $(BUILD_DIR)/kernel.elf \
	    -ex "target remote localhost:1234" \
	    -ex "break kernel_main" \
	    -ex "continue"

clean:
	rm -rf $(BUILD_DIR) $(ISO_DIR) os.img os.iso

.PHONY: all run debug clean
```

### Linker Script (`linker.ld`)
```ld
ENTRY(kernel_entry)

SECTIONS
{
    . = 0x100000;  /* Kernel loads at 1MB */
    
    .text : {
        *(.text)
    }
    
    .rodata : {
        *(.rodata)
    }
    
    .data : {
        *(.data)
    }
    
    .bss : {
        __bss_start = .;
        *(.bss)
        *(COMMON)
        __bss_end = .;
    }
    
    __kernel_end = .;
}
```

---

## Testing Commands

```bash
# Build everything
make clean && make

# Run in QEMU with serial output
make run

# Debug with GDB
make debug

# In another terminal, monitor QEMU
telnet localhost 45454
(qemu) info registers
(qemu) info mem
(qemu) x/10i $pc  # Show next 10 instructions
```

## Key Learning Outcomes

1. **CPU Boot Process** - Real mode → Protected mode → Long mode
2. **Memory Management** - Physical pages, virtual memory, paging
3. **Interrupt Handling** - Hardware/software interrupts, IDT
4. **Process Management** - Context switching, scheduling
5. **System Calls** - User/kernel boundary
6. **Device Drivers** - Direct hardware control
7. **Build Systems** - Cross-compilation, linking

## Next Steps

After completing the basic OS:
- Add file system support (FAT32 or custom)
- Implement networking (TCP/IP stack)
- Add more device drivers (USB, sound, etc.)
- Implement user authentication
- Add GUI support
- Port existing software

This plan creates a functional OS that boots, manages memory, handles interrupts, runs multiple processes, and provides a shell interface - all written from scratch!