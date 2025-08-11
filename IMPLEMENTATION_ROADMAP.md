# OS From Scratch - Implementation Roadmap

## Current Status
✅ **Documentation Complete**
- Master plan created with all 9 phases detailed
- CPU architecture guide written
- OS architecture diagrams completed
- Process scheduler simulator implemented

❌ **Implementation Not Started**
- No code directories created yet
- Development tools not installed
- No bootloader or kernel code written

## Phase-by-Phase Implementation Plan

### PHASE 0: Environment Setup (Prerequisite)
**Duration**: 1 hour
**Dependencies**: None
**Deliverables**:
- [x] Install development tools (nasm, qemu, ~~x86_64-elf-gcc~~, make) ✅
- [x] Create project directory structure ✅
- [x] Setup Makefile ~~and linker script~~ ✅
- [x] Test toolchain with hello world assembly ✅

**Key Commands**:
```bash
# Install tools on macOS
brew install nasm qemu x86_64-elf-gcc x86_64-elf-binutils make

# Create project structure
mkdir -p boot kernel drivers mm include build

# Test assembly
nasm -v
qemu-system-x86_64 --version
```

---

### PHASE 1: Minimal Boot Sector
**Duration**: 2-3 hours
**Dependencies**: Phase 0 complete
**Deliverables**:
- [ ] `boot/boot.asm` - 512-byte boot sector
- [ ] Prints "Booting OS..." message
- [ ] Boots successfully in QEMU
- [ ] Boot signature (0xAA55) verified

**Success Criteria**:
- QEMU boots and displays message
- No BIOS errors
- Stable halt after message

**Test Command**:
```bash
make boot && qemu-system-x86_64 -drive format=raw,file=build/boot.bin
```

---

### PHASE 2: Stage 2 Bootloader
**Duration**: 4-5 hours
**Dependencies**: Phase 1 working
**Deliverables**:
- [ ] `boot/stage2.asm` - Extended bootloader
- [ ] A20 line enabled
- [ ] GDT (Global Descriptor Table) setup
- [ ] Switch to 32-bit protected mode
- [ ] Load kernel preparation

**Key Milestones**:
1. Successfully enable A20 line
2. Load and activate GDT
3. Switch CPU to protected mode
4. Jump to 32-bit code segment

---

### PHASE 3: 64-bit Long Mode
**Duration**: 3-4 hours
**Dependencies**: Phase 2 complete
**Deliverables**:
- [ ] `boot/long_mode.asm` - 64-bit transition
- [ ] CPU feature detection (check for 64-bit support)
- [ ] Page tables setup (PML4, PDPT, PDT, PT)
- [ ] Enable paging
- [ ] Switch to long mode

**Critical Steps**:
1. Check CPUID for long mode support
2. Setup 4-level page tables
3. Enable PAE and long mode bits
4. Jump to 64-bit code

---

### PHASE 4: C Kernel Entry
**Duration**: 3-4 hours
**Dependencies**: Phase 3 working
**Deliverables**:
- [ ] `kernel/entry.asm` - Kernel entry point
- [ ] `kernel/kernel.c` - Main kernel code
- [ ] `drivers/vga.c` - Screen output driver
- [ ] Working VGA text output
- [ ] "Welcome to our OS!" displayed

**Integration Points**:
- Assembly to C transition
- Stack setup for C code
- BSS section clearing
- First C function call

---

### PHASE 5: Interrupt System
**Duration**: 5-6 hours
**Dependencies**: Phase 4 complete
**Deliverables**:
- [ ] `kernel/idt.c` - Interrupt Descriptor Table
- [ ] `kernel/interrupt.asm` - Interrupt handlers
- [ ] `drivers/timer.c` - Timer interrupts
- [ ] `drivers/keyboard.c` - Keyboard input
- [ ] Exception handling (divide by zero, page fault, etc.)

**Test Cases**:
- Timer tick counter increments
- Keyboard input echoes to screen
- Exception handler catches divide by zero
- Clean interrupt return

---

### PHASE 6: Memory Management
**Duration**: 6-8 hours
**Dependencies**: Phase 5 stable
**Deliverables**:
- [ ] `mm/pmm.c` - Physical memory manager
- [ ] `mm/vmm.c` - Virtual memory manager
- [ ] `mm/heap.c` - Heap allocator (malloc/free)
- [ ] Bitmap-based page allocation
- [ ] Page mapping/unmapping

**Memory Layout**:
```
0x000000 - 0x0FFFFF : BIOS/Legacy (1MB)
0x100000 - 0x1FFFFF : Kernel (1MB)
0x200000 - 0x3FFFFF : Kernel heap (2MB)
0x400000 - ...      : User space
```

---

### PHASE 7: Process Management
**Duration**: 6-8 hours
**Dependencies**: Phase 6 working
**Deliverables**:
- [ ] `kernel/process.c` - Process structures
- [ ] `kernel/scheduler.c` - Round-robin scheduler
- [ ] `kernel/context.asm` - Context switching
- [ ] Multiple process execution
- [ ] Process states (ready, running, blocked)

**Key Features**:
- Process Control Block (PCB)
- Context save/restore
- Timer-based preemption
- Ready queue management

---

### PHASE 8: System Calls
**Duration**: 4-5 hours
**Dependencies**: Phase 7 complete
**Deliverables**:
- [ ] `kernel/syscall.c` - System call handler
- [ ] `kernel/syscall_entry.asm` - INT 0x80 handler
- [ ] Basic syscalls: exit, write, read
- [ ] User/kernel mode separation
- [ ] Syscall table

**Syscalls to Implement**:
1. sys_exit - Terminate process
2. sys_write - Output to screen
3. sys_read - Input from keyboard
4. sys_fork - Create process
5. sys_exec - Execute program

---

### PHASE 9: Simple Shell
**Duration**: 4-5 hours
**Dependencies**: Phase 8 working
**Deliverables**:
- [ ] `shell/shell.c` - Command interpreter
- [ ] Built-in commands (help, clear, ps, mem)
- [ ] Command parsing
- [ ] Process launching
- [ ] Interactive prompt

**Shell Commands**:
- `help` - Show available commands
- `clear` - Clear screen
- `ps` - List processes
- `mem` - Memory statistics
- `echo` - Print arguments

---

## Implementation Order & Dependencies

```
Phase 0: Environment Setup
    ↓
Phase 1: Boot Sector (Real Mode)
    ↓
Phase 2: Stage 2 Bootloader (→ Protected Mode)
    ↓
Phase 3: Long Mode Setup (→ 64-bit)
    ↓
Phase 4: C Kernel & VGA Driver
    ↓
Phase 5: Interrupts (IDT, Timer, Keyboard)
    ↓
Phase 6: Memory Management (PMM, VMM, Heap)
    ↓
Phase 7: Process Management (Scheduler, Context Switch)
    ↓
Phase 8: System Calls (User/Kernel Interface)
    ↓
Phase 9: Shell (User Interface)
```

## Testing Strategy

### Unit Testing
Each phase should be tested independently:
- Phase 1: Boot message appears
- Phase 2: Protected mode status check
- Phase 3: Long mode active verification
- Phase 4: VGA output working
- Phase 5: Interrupts firing correctly
- Phase 6: Memory allocation/deallocation
- Phase 7: Process switching
- Phase 8: Syscall execution
- Phase 9: Shell commands work

### Integration Testing
After each phase, test with all previous phases:
- Boot → Protected → Long → Kernel flow
- Interrupts with memory management
- Processes using memory and syscalls
- Shell using all kernel services

### Debug Tools
```bash
# QEMU monitor commands
(qemu) info registers  # Check CPU state
(qemu) info mem       # Memory mappings
(qemu) x/10i $pc     # Disassemble at PC
(qemu) gva2gpa ADDR  # Virtual to physical

# GDB debugging
gdb kernel.elf
target remote :1234
break kernel_main
continue
```

## Common Issues & Solutions

### Issue: "No bootable device"
**Solution**: Check boot signature (0xAA55) at bytes 510-511

### Issue: Triple fault / reboot loop
**Solution**: Check GDT setup, stack pointer, page tables

### Issue: Page fault
**Solution**: Verify page mappings, check CR3 register

### Issue: Keyboard not working
**Solution**: Check IRQ remapping, keyboard controller init

### Issue: Process crash
**Solution**: Verify context save/restore, stack alignment

## Next Session Checklist

Before starting implementation:
1. ✅ Review master plan
2. ✅ Understand CPU boot process
3. ✅ Know memory layout
4. ⬜ Install development tools
5. ⬜ Create project structure
6. ⬜ Write initial Makefile
7. ⬜ Start with Phase 1 boot sector

## Success Metrics

- **Phase 1-3**: OS boots to 64-bit mode
- **Phase 4-5**: Kernel runs with interrupts
- **Phase 6-7**: Multiple processes execute
- **Phase 8-9**: Interactive shell works

Total estimated time: 40-50 hours of focused development