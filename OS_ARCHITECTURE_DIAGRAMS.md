# OS Architecture and Hardware Interaction Diagrams

## Overall System Architecture

```mermaid
graph TB
    subgraph "User Space"
        SHELL[Shell]
        USER_PROC[User Processes]
        USER_LIB[User Libraries]
    end
    
    subgraph "Kernel Space"
        SYSCALL[System Call Interface]
        VFS[Virtual File System]
        PROC_MGR[Process Manager]
        MEM_MGR[Memory Manager]
        SCHED[Scheduler]
        IPC[IPC Manager]
        
        subgraph "Device Drivers"
            KBD_DRV[Keyboard Driver]
            VGA_DRV[VGA Driver]
            DISK_DRV[Disk Driver]
            TIMER_DRV[Timer Driver]
        end
    end
    
    subgraph "Hardware Abstraction Layer"
        INT_HANDLER[Interrupt Handler]
        PORT_IO[Port I/O]
        MMU[Memory Management Unit]
    end
    
    subgraph "Hardware"
        CPU[CPU x86_64]
        RAM[RAM]
        DISK[Hard Disk]
        KBD[Keyboard]
        SCREEN[Screen/VGA]
        TIMER[PIT Timer]
        PIC[PIC 8259]
    end
    
    SHELL --> SYSCALL
    USER_PROC --> SYSCALL
    USER_LIB --> SYSCALL
    
    SYSCALL --> VFS
    SYSCALL --> PROC_MGR
    SYSCALL --> MEM_MGR
    
    PROC_MGR --> SCHED
    PROC_MGR --> MEM_MGR
    
    SCHED --> TIMER_DRV
    
    VFS --> DISK_DRV
    
    KBD_DRV --> INT_HANDLER
    VGA_DRV --> PORT_IO
    DISK_DRV --> PORT_IO
    TIMER_DRV --> INT_HANDLER
    
    INT_HANDLER --> PIC
    PORT_IO --> CPU
    MMU --> CPU
    
    PIC --> CPU
    CPU --> RAM
    DISK_DRV --> DISK
    KBD_DRV --> KBD
    VGA_DRV --> SCREEN
    TIMER_DRV --> TIMER
```

## Boot Process Flow

```mermaid
sequenceDiagram
    participant BIOS
    participant BOOT as Boot Sector
    participant STAGE2 as Stage 2 Loader
    participant KERNEL as Kernel
    participant INIT as Init Process
    participant SHELL as Shell
    
    BIOS->>BIOS: POST (Power On Self Test)
    BIOS->>BIOS: Load first 512 bytes from disk
    BIOS->>BOOT: Jump to 0x7C00
    
    BOOT->>BOOT: Setup segments
    BOOT->>BOOT: Print boot message
    BOOT->>STAGE2: Load Stage 2 from disk
    BOOT->>STAGE2: Jump to Stage 2
    
    STAGE2->>STAGE2: Enable A20 line
    STAGE2->>STAGE2: Setup GDT
    STAGE2->>STAGE2: Switch to Protected Mode
    STAGE2->>STAGE2: Setup paging
    STAGE2->>STAGE2: Switch to Long Mode
    STAGE2->>KERNEL: Load kernel from disk
    STAGE2->>KERNEL: Jump to kernel entry
    
    KERNEL->>KERNEL: Initialize BSS
    KERNEL->>KERNEL: Setup stack
    KERNEL->>KERNEL: Initialize VGA
    KERNEL->>KERNEL: Setup GDT/IDT
    KERNEL->>KERNEL: Initialize memory manager
    KERNEL->>KERNEL: Initialize scheduler
    KERNEL->>KERNEL: Setup drivers
    KERNEL->>INIT: Create init process
    
    INIT->>SHELL: Launch shell
    SHELL->>SHELL: Command loop
```

## Memory Layout

```mermaid
graph TB
    subgraph "Virtual Memory Layout (64-bit)"
        VM_KERNEL["0x0000000000000000 - 0x00000000FFFFFFFF<br/>Kernel Space (4GB)<br/>- Kernel Code<br/>- Kernel Data<br/>- Drivers<br/>- Page Tables"]
        VM_HEAP["0x0000000100000000 - 0x00000001FFFFFFFF<br/>Kernel Heap (4GB)"]
        VM_MMIO["0x0000000200000000 - 0x00000002FFFFFFFF<br/>Memory Mapped I/O (4GB)"]
        VM_USER["0x0000010000000000 - 0x00007FFFFFFFFFFF<br/>User Space<br/>- User Code<br/>- User Data<br/>- User Stack<br/>- User Heap"]
        VM_UNUSED["0x0000800000000000 - 0xFFFFFFFFFFFFFFFF<br/>Unused/Reserved"]
    end
    
    subgraph "Physical Memory Layout"
        PM_BIOS["0x00000000 - 0x000003FF<br/>IVT (1KB)"]
        PM_BDA["0x00000400 - 0x000004FF<br/>BIOS Data Area"]
        PM_FREE1["0x00000500 - 0x00007BFF<br/>Free (30KB)"]
        PM_BOOT["0x00007C00 - 0x00007DFF<br/>Boot Sector (512B)"]
        PM_STAGE2["0x00007E00 - 0x0009FFFF<br/>Stage 2 Loader"]
        PM_EBDA["0x000A0000 - 0x000BFFFF<br/>Video Memory (128KB)"]
        PM_BIOS_ROM["0x000C0000 - 0x000FFFFF<br/>BIOS ROM (256KB)"]
        PM_KERNEL["0x00100000 - 0x001FFFFF<br/>Kernel (1MB)"]
        PM_FREE2["0x00200000 - END<br/>Free RAM"]
    end
    
    VM_KERNEL -.->|"Maps to"| PM_KERNEL
    VM_USER -.->|"Maps to"| PM_FREE2
```

## Interrupt and System Call Flow

```mermaid
graph LR
    subgraph "User Mode"
        USER[User Process]
    end
    
    subgraph "Transition"
        INT80[INT 0x80<br/>Software Interrupt]
        HW_INT[Hardware<br/>Interrupt]
    end
    
    subgraph "Kernel Mode"
        IDT[IDT Entry]
        ISR[Interrupt Service Routine]
        SYSCALL_H[Syscall Handler]
        IRQ_H[IRQ Handler]
        
        subgraph "Handlers"
            TIMER_H[Timer Handler]
            KBD_H[Keyboard Handler]
            PF_H[Page Fault Handler]
        end
    end
    
    USER -->|System Call| INT80
    INT80 --> IDT
    IDT --> SYSCALL_H
    SYSCALL_H -->|Return| USER
    
    HW_INT --> IDT
    IDT --> ISR
    ISR --> IRQ_H
    IRQ_H --> TIMER_H
    IRQ_H --> KBD_H
    ISR --> PF_H
```

## Process State Machine

```mermaid
stateDiagram-v2
    [*] --> NEW: Process Created
    NEW --> READY: Admitted
    READY --> RUNNING: Scheduler Dispatch
    RUNNING --> READY: Timer Interrupt/<br/>Preemption
    RUNNING --> BLOCKED: I/O or Wait
    BLOCKED --> READY: I/O Complete/<br/>Event Occurs
    RUNNING --> ZOMBIE: Exit
    ZOMBIE --> [*]: Parent Reaps
```

## Page Table Structure (4-Level Paging)

```mermaid
graph TB
    CR3[CR3 Register]
    PML4[PML4 Table<br/>512 entries]
    PDPT[PDPT<br/>512 entries]
    PD[Page Directory<br/>512 entries]
    PT[Page Table<br/>512 entries]
    PAGE[Physical Page<br/>4KB]
    
    VADDR[Virtual Address<br/>48 bits used]
    
    VADDR -->|"Bits 39-47"| PML4
    VADDR -->|"Bits 30-38"| PDPT
    VADDR -->|"Bits 21-29"| PD
    VADDR -->|"Bits 12-20"| PT
    VADDR -->|"Bits 0-11"| PAGE
    
    CR3 --> PML4
    PML4 --> PDPT
    PDPT --> PD
    PD --> PT
    PT --> PAGE
```

## Driver-Hardware Interaction

```mermaid
sequenceDiagram
    participant APP as Application
    participant KERNEL as Kernel
    participant DRIVER as Device Driver
    participant HAL as Hardware Abstraction
    participant HW as Hardware Device
    
    APP->>KERNEL: System Call (read/write)
    KERNEL->>DRIVER: Driver Function
    DRIVER->>HAL: Port I/O Request
    HAL->>HW: OUT/IN instruction
    HW-->>HAL: Data/Status
    HAL-->>DRIVER: Return Value
    
    Note over HW: Hardware Interrupt
    HW->>HAL: IRQ Signal
    HAL->>DRIVER: Interrupt Handler
    DRIVER->>DRIVER: Process Data
    DRIVER->>KERNEL: Wake Waiting Process
    KERNEL->>APP: Return from Syscall
```

## Scheduler Algorithm (Round Robin)

```mermaid
graph TB
    START[Timer Interrupt]
    SAVE[Save Current Process Context]
    CHECK{Ready Queue<br/>Empty?}
    IDLE[Run Idle Task]
    DEQUEUE[Dequeue Next Process]
    SWITCH[Context Switch]
    LOAD[Load New Context]
    RUN[Run Process]
    
    START --> SAVE
    SAVE --> CHECK
    CHECK -->|Yes| IDLE
    CHECK -->|No| DEQUEUE
    DEQUEUE --> SWITCH
    SWITCH --> LOAD
    LOAD --> RUN
    IDLE --> RUN
```

## Memory Allocation Strategy

```mermaid
graph TB
    REQUEST[malloc request]
    SIZE[Align size to 8 bytes]
    SEARCH[Search free list]
    FOUND{Block<br/>found?}
    SPLIT{Block too<br/>large?}
    SPLIT_BLOCK[Split block]
    MARK_USED[Mark as used]
    EXPAND[Expand heap]
    NEW_PAGE[Allocate new page]
    MAP[Map page to virtual memory]
    RETURN[Return pointer]
    
    REQUEST --> SIZE
    SIZE --> SEARCH
    SEARCH --> FOUND
    FOUND -->|Yes| SPLIT
    FOUND -->|No| EXPAND
    SPLIT -->|Yes| SPLIT_BLOCK
    SPLIT -->|No| MARK_USED
    SPLIT_BLOCK --> MARK_USED
    MARK_USED --> RETURN
    EXPAND --> NEW_PAGE
    NEW_PAGE --> MAP
    MAP --> SEARCH
```

## I/O Port Communication

```mermaid
sequenceDiagram
    participant CPU
    participant PORT as I/O Port
    participant DEV as Device
    
    Note over CPU,DEV: Keyboard Input Example
    DEV->>PORT: Key pressed signal
    PORT->>CPU: IRQ 1
    CPU->>PORT: IN 0x60 (read scancode)
    PORT-->>CPU: Scancode value
    CPU->>CPU: Process scancode
    
    Note over CPU,DEV: VGA Output Example
    CPU->>PORT: OUT 0x3D4, 0x0F (cursor low)
    CPU->>PORT: OUT 0x3D5, position_low
    CPU->>PORT: OUT 0x3D4, 0x0E (cursor high)
    CPU->>PORT: OUT 0x3D5, position_high
    PORT->>DEV: Update cursor position
```

## Build Process Flow

```mermaid
graph LR
    subgraph "Source Files"
        ASM[Assembly Files<br/>.asm]
        C[C Files<br/>.c]
    end
    
    subgraph "Compilation"
        NASM[NASM<br/>Assembler]
        GCC[GCC<br/>Compiler]
    end
    
    subgraph "Object Files"
        ASM_OBJ[Assembly Objects<br/>.o]
        C_OBJ[C Objects<br/>.o]
    end
    
    subgraph "Linking"
        LD[LD Linker]
        LINKER_SCRIPT[Linker Script]
    end
    
    subgraph "Binary"
        ELF[Kernel ELF]
        BIN[Kernel Binary]
        IMG[OS Image]
    end
    
    ASM --> NASM
    C --> GCC
    NASM --> ASM_OBJ
    GCC --> C_OBJ
    ASM_OBJ --> LD
    C_OBJ --> LD
    LINKER_SCRIPT --> LD
    LD --> ELF
    ELF --> BIN
    BIN --> IMG
```

## System Call Mechanism

```mermaid
sequenceDiagram
    participant USER as User Process
    participant CPU
    participant IDT
    participant HANDLER as Syscall Handler
    participant KERNEL as Kernel Service
    
    USER->>USER: Setup parameters in registers
    USER->>CPU: INT 0x80
    CPU->>CPU: Switch to Ring 0
    CPU->>IDT: Lookup handler for 0x80
    IDT->>HANDLER: Jump to handler
    HANDLER->>HANDLER: Save user context
    HANDLER->>HANDLER: Extract syscall number
    HANDLER->>KERNEL: Call appropriate service
    KERNEL->>KERNEL: Perform operation
    KERNEL-->>HANDLER: Return value
    HANDLER->>HANDLER: Restore user context
    HANDLER->>CPU: IRET instruction
    CPU->>CPU: Switch to Ring 3
    CPU-->>USER: Return to user code
```

## CPU Mode Transitions

```mermaid
graph TD
    REAL["Real Mode<br/>16-bit<br/>1MB RAM limit<br/>No protection"]
    PROT["Protected Mode<br/>32-bit<br/>4GB RAM<br/>Memory protection"]
    LONG["Long Mode<br/>64-bit<br/>Huge RAM<br/>Enhanced features"]
    
    REAL -->|"Set CR0.PE=1"| PROT
    PROT -->|"Enable PAE<br/>Set EFER.LME=1<br/>Enable Paging"| LONG
    
    subgraph "Real Mode Setup"
        R1[Load segments]
        R2[Setup stack]
        R3[Enable A20]
    end
    
    subgraph "Protected Mode Setup"
        P1[Load GDT]
        P2[Set segment selectors]
        P3[Setup 32-bit stack]
    end
    
    subgraph "Long Mode Setup"
        L1[Setup 4-level paging]
        L2[Load 64-bit GDT]
        L3[Jump to 64-bit code]
    end
```

## Hardware Abstraction Layers

```mermaid
graph TB
    subgraph "Application Layer"
        APP1[Text Editor]
        APP2[Calculator]
        APP3[Games]
    end
    
    subgraph "System Call Layer"
        SC1[read/write]
        SC2[malloc/free]
        SC3[fork/exec]
    end
    
    subgraph "Kernel Services"
        FS[File System]
        MM[Memory Manager]
        PS[Process Scheduler]
    end
    
    subgraph "Device Drivers"
        STOR[Storage Driver]
        NET[Network Driver]
        DISP[Display Driver]
    end
    
    subgraph "Hardware Abstraction"
        PIO[Port I/O]
        MMIO[Memory Mapped I/O]
        INT[Interrupts]
    end
    
    subgraph "Physical Hardware"
        HDD[Hard Disk]
        NIC[Network Card]
        GPU[Graphics Card]
    end
    
    APP1 --> SC1
    APP2 --> SC2
    APP3 --> SC3
    
    SC1 --> FS
    SC2 --> MM
    SC3 --> PS
    
    FS --> STOR
    PS --> NET
    MM --> DISP
    
    STOR --> PIO
    NET --> MMIO
    DISP --> INT
    
    PIO --> HDD
    MMIO --> NIC
    INT --> GPU
```

These diagrams provide a comprehensive visual representation of:
1. Overall system architecture and component relationships
2. Boot process sequence
3. Memory organization (virtual and physical)
4. Interrupt and system call flow
5. Process state transitions
6. Page table structure for virtual memory
7. Driver-hardware communication
8. Scheduler operation
9. Memory allocation strategy
10. I/O port communication
11. Build process
12. System call mechanism
13. CPU mode transitions
14. Hardware abstraction layers

Each diagram shows how different parts of our OS will interact with the hardware and with each other, providing clear visualization of the complex relationships in our operating system.