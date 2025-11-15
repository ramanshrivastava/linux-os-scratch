# KVM API Reference Guide

## Overview

The KVM API is accessed through **ioctl** system calls on file descriptors. This document organizes all KVM ioctls by category and provides detailed information about each.

## Table of Contents

1. [API Basics](#api-basics)
2. [System ioctls](#system-ioctls)
3. [VM ioctls](#vm-ioctls)
4. [vCPU ioctls](#vcpu-ioctls)
5. [Device ioctls](#device-ioctls)
6. [Memory Management](#memory-management)
7. [Interrupt Handling](#interrupt-handling)
8. [Quick Reference Table](#quick-reference-table)

---

## API Basics

### File Descriptor Hierarchy

```
/dev/kvm (system FD)
    ├─ KVM_CREATE_VM → VM FD
    │   ├─ KVM_CREATE_VCPU → vCPU FD (0)
    │   ├─ KVM_CREATE_VCPU → vCPU FD (1)
    │   └─ KVM_CREATE_DEVICE → Device FD
    └─ System-level queries
```

### API Version

**Current Stable Version**: 12 (since Linux 2.6.22)

```c
int kvm = open("/dev/kvm", O_RDWR);
int version = ioctl(kvm, KVM_GET_API_VERSION, 0);
if (version != 12) {
    fprintf(stderr, "KVM API version mismatch\n");
    exit(1);
}
```

### Extension Mechanism

KVM uses a capability-based extension system. Check capabilities before using advanced features:

```c
int cap = ioctl(kvm, KVM_CHECK_EXTENSION, KVM_CAP_USER_MEMORY);
if (cap > 0) {
    // Feature is supported
}
```

---

## System ioctls

System ioctls operate on `/dev/kvm` file descriptor and affect the KVM subsystem globally.

### 4.1 KVM_GET_API_VERSION

**Purpose**: Get the KVM API version number

```c
Capability: basic
Architectures: all
Type: system ioctl
Parameters: none
Returns: 12 (current stable version)
```

**Example**:
```c
int kvm_fd = open("/dev/kvm", O_RDWR);
int version = ioctl(kvm_fd, KVM_GET_API_VERSION, 0);
printf("KVM API version: %d\n", version);
```

---

### 4.2 KVM_CREATE_VM

**Purpose**: Create a new virtual machine

```c
Capability: basic
Architectures: all
Type: system ioctl
Parameters: machine type identifier (usually 0)
Returns: VM file descriptor (>=0 on success, -1 on error)
```

**Machine Types**:
- `0`: Default (recommended for most use cases)
- `KVM_VM_S390_UCONTROL`: S390 user-controlled VMs
- `KVM_VM_MIPS_VZ`: MIPS VZ hardware virtualization
- `KVM_VM_TYPE_ARM_IPA_SIZE(N)`: ARM64 with custom IPA size

**Example**:
```c
int vm_fd = ioctl(kvm_fd, KVM_CREATE_VM, 0);
if (vm_fd < 0) {
    perror("KVM_CREATE_VM");
    exit(1);
}
```

**ARM64 IPA Size Example**:
```c
// Configure 48-bit physical address space for ARM64 VM
int vm_fd = ioctl(kvm_fd, KVM_CREATE_VM, KVM_VM_TYPE_ARM_IPA_SIZE(48));
```

---

### 4.3 KVM_GET_MSR_INDEX_LIST / KVM_GET_MSR_FEATURE_INDEX_LIST

**Purpose**: Get list of supported MSRs (Model Specific Registers)

```c
Capability: basic (KVM_CAP_GET_MSR_FEATURES for feature list)
Architectures: x86
Type: system ioctl
Parameters: struct kvm_msr_list (in/out)
Returns: 0 on success, -1 on error
```

**Structure**:
```c
struct kvm_msr_list {
    __u32 nmsrs;          /* number of MSRs */
    __u32 indices[0];     /* MSR indices */
};
```

**Example**:
```c
struct kvm_msr_list *msr_list;
int nmsrs = 100;

msr_list = calloc(1, sizeof(*msr_list) + nmsrs * sizeof(__u32));
msr_list->nmsrs = nmsrs;

if (ioctl(kvm_fd, KVM_GET_MSR_INDEX_LIST, msr_list) < 0) {
    if (errno == E2BIG) {
        // List too large, retry with msr_list->nmsrs
        nmsrs = msr_list->nmsrs;
        free(msr_list);
        // Reallocate and retry...
    }
}
```

**Use Cases**:
- Determine which MSRs can be read/written
- Check CPU feature availability
- Configure CPUID emulation

---

### 4.4 KVM_CHECK_EXTENSION

**Purpose**: Query support for KVM extensions/capabilities

```c
Capability: basic
Architectures: all
Type: system ioctl, vm ioctl
Parameters: extension identifier (KVM_CAP_*)
Returns: 0 if unsupported, >0 if supported (may indicate version)
```

**Common Capabilities**:

| Capability | Description | Return Value |
|------------|-------------|--------------|
| `KVM_CAP_USER_MEMORY` | User-provided memory regions | 1 if supported |
| `KVM_CAP_NR_VCPUS` | Recommended max vCPUs | Number of vCPUs |
| `KVM_CAP_MAX_VCPUS` | Maximum possible vCPUs | Number of vCPUs |
| `KVM_CAP_IRQCHIP` | In-kernel interrupt controller | 1 if supported |
| `KVM_CAP_SYNC_REGS` | Register synchronization | Bitmask of supported regs |

**Example**:
```c
// Check if in-kernel IRQCHIP is supported
int cap = ioctl(kvm_fd, KVM_CHECK_EXTENSION, KVM_CAP_IRQCHIP);
if (cap > 0) {
    printf("In-kernel IRQCHIP is supported\n");
}

// Get recommended number of vCPUs
int max_vcpus = ioctl(kvm_fd, KVM_CHECK_EXTENSION, KVM_CAP_NR_VCPUS);
if (max_vcpus < 0) {
    max_vcpus = 4; // Default if capability not present
}
printf("Recommended max vCPUs: %d\n", max_vcpus);
```

---

### 4.5 KVM_GET_VCPU_MMAP_SIZE

**Purpose**: Get size of shared vCPU state region

```c
Capability: basic
Architectures: all
Type: system ioctl
Parameters: none
Returns: size in bytes (>0 on success)
```

**Purpose**: The `KVM_RUN` ioctl uses a memory-mapped region to communicate with userspace. This ioctl returns the size of that region.

**Example**:
```c
int vcpu_mmap_size = ioctl(kvm_fd, KVM_GET_VCPU_MMAP_SIZE, 0);
if (vcpu_mmap_size < 0) {
    perror("KVM_GET_VCPU_MMAP_SIZE");
    exit(1);
}

// Later, after creating vCPU:
struct kvm_run *run = mmap(NULL, vcpu_mmap_size,
                           PROT_READ | PROT_WRITE,
                           MAP_SHARED, vcpu_fd, 0);
```

---

### 4.46 KVM_GET_SUPPORTED_CPUID

**Purpose**: Get CPUID features supported by KVM

```c
Capability: KVM_CAP_EXT_CPUID
Architectures: x86
Type: system ioctl
Parameters: struct kvm_cpuid2 (in/out)
Returns: 0 on success, -1 on error
```

**Structures**:
```c
struct kvm_cpuid2 {
    __u32 nent;                      /* number of entries */
    __u32 padding;
    struct kvm_cpuid_entry2 entries[0];
};

struct kvm_cpuid_entry2 {
    __u32 function;                  /* CPUID function (EAX) */
    __u32 index;                     /* CPUID index (ECX) */
    __u32 flags;
    __u32 eax;                       /* CPUID output values */
    __u32 ebx;
    __u32 ecx;
    __u32 edx;
    __u32 padding[3];
};
```

**Example**:
```c
struct kvm_cpuid2 *cpuid;
int nent = 100;

cpuid = calloc(1, sizeof(*cpuid) + nent * sizeof(cpuid->entries[0]));
cpuid->nent = nent;

if (ioctl(kvm_fd, KVM_GET_SUPPORTED_CPUID, cpuid) < 0) {
    if (errno == E2BIG) {
        // Need more entries
        nent = cpuid->nent;
        // Reallocate and retry...
    }
}

// Use cpuid information to configure guest
```

---

## VM ioctls

VM ioctls operate on the VM file descriptor returned by `KVM_CREATE_VM`.

### 4.7 KVM_CREATE_VCPU

**Purpose**: Create a virtual CPU for the VM

```c
Capability: basic
Architectures: all
Type: vm ioctl
Parameters: vcpu id (integer)
Returns: vCPU file descriptor (>=0 on success, -1 on error)
```

**vCPU ID Range**: `[0, max_vcpu_id)`
- Check `KVM_CAP_MAX_VCPU_ID` for maximum ID
- Check `KVM_CAP_NR_VCPUS` for recommended max vCPUs
- Check `KVM_CAP_MAX_VCPUS` for absolute maximum

**Example**:
```c
// Query maximum vCPUs
int max_vcpus = ioctl(kvm_fd, KVM_CHECK_EXTENSION, KVM_CAP_NR_VCPUS);
if (max_vcpus < 0) max_vcpus = 4;

// Create vCPUs
int vcpu_fds[max_vcpus];
for (int i = 0; i < max_vcpus; i++) {
    vcpu_fds[i] = ioctl(vm_fd, KVM_CREATE_VCPU, i);
    if (vcpu_fds[i] < 0) {
        perror("KVM_CREATE_VCPU");
        exit(1);
    }
}
```

**PowerPC Note**: vCPUs may be mapped onto virtual threads in CPU cores.

---

### 4.35 KVM_SET_USER_MEMORY_REGION

**Purpose**: Map guest physical memory to host virtual memory

```c
Capability: KVM_CAP_USER_MEMORY
Architectures: all
Type: vm ioctl
Parameters: struct kvm_userspace_memory_region (in)
Returns: 0 on success, -1 on error
```

**Structure**:
```c
struct kvm_userspace_memory_region {
    __u32 slot;                      /* Memory slot ID */
    __u32 flags;                     /* KVM_MEM_LOG_DIRTY_PAGES | KVM_MEM_READONLY */
    __u64 guest_phys_addr;           /* Guest physical address */
    __u64 memory_size;               /* Size in bytes */
    __u64 userspace_addr;            /* Userspace virtual address */
};
```

**Flags**:
- `KVM_MEM_LOG_DIRTY_PAGES`: Track dirty pages for live migration
- `KVM_MEM_READONLY`: Make memory read-only (writes → MMIO exits)

**Example**:
```c
// Allocate 1GB of host memory for guest RAM
size_t mem_size = 1UL << 30;  // 1GB
void *mem = mmap(NULL, mem_size, PROT_READ | PROT_WRITE,
                 MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
if (mem == MAP_FAILED) {
    perror("mmap");
    exit(1);
}

struct kvm_userspace_memory_region region = {
    .slot = 0,
    .flags = 0,
    .guest_phys_addr = 0x0,
    .memory_size = mem_size,
    .userspace_addr = (unsigned long)mem
};

if (ioctl(vm_fd, KVM_SET_USER_MEMORY_REGION, &region) < 0) {
    perror("KVM_SET_USER_MEMORY_REGION");
    exit(1);
}
```

**Important Notes**:
- To delete a slot, set `memory_size = 0`
- Slots cannot overlap in guest physical address space
- For best performance, align to 2MB boundaries (huge pages)
- Can move a slot by changing `guest_phys_addr` or `userspace_addr`
- With `KVM_CAP_MULTI_ADDRESS_SPACE`, use bits 16-31 of `slot` for address space ID

---

### 4.8 KVM_GET_DIRTY_LOG

**Purpose**: Get bitmap of pages modified by the guest

```c
Capability: basic
Architectures: all
Type: vm ioctl
Parameters: struct kvm_dirty_log (in/out)
Returns: 0 on success, -1 on error
```

**Structure**:
```c
struct kvm_dirty_log {
    __u32 slot;                      /* Memory slot to query */
    __u32 padding;
    union {
        void __user *dirty_bitmap;   /* One bit per page */
        __u64 padding;
    };
};
```

**Example (Live Migration)**:
```c
unsigned long num_pages = mem_size / 4096;
unsigned long bitmap_size = (num_pages + 63) / 64 * 8;  // Round up to 8-byte boundary
unsigned char *bitmap = malloc(bitmap_size);

struct kvm_dirty_log log = {
    .slot = 0,
    .dirty_bitmap = bitmap
};

// Get dirty pages
if (ioctl(vm_fd, KVM_GET_DIRTY_LOG, &log) < 0) {
    perror("KVM_GET_DIRTY_LOG");
    exit(1);
}

// Check which pages are dirty
for (unsigned long i = 0; i < num_pages; i++) {
    if (bitmap[i / 8] & (1 << (i % 8))) {
        printf("Page %lu is dirty\n", i);
        // Transmit page to destination in live migration
    }
}
```

**Usage in Cloud**:
- **Live Migration**: Transfer only modified pages
- **Checkpointing**: Save VM state incrementally
- **Debugging**: Track memory access patterns

---

### 4.24 KVM_CREATE_IRQCHIP

**Purpose**: Create in-kernel interrupt controller

```c
Capability: KVM_CAP_IRQCHIP
Architectures: x86, ARM, arm64, s390
Type: vm ioctl
Parameters: none
Returns: 0 on success, -1 on error
```

**What It Creates**:
- **x86**: Virtual IOAPIC, dual PICs, local APIC for each vCPU
- **ARM/arm64**: GICv2 (use `KVM_CREATE_DEVICE` for GICv3+)
- **s390**: Dummy IRQ routing table

**Example**:
```c
// Must be called before creating vCPUs for full functionality
if (ioctl(vm_fd, KVM_CREATE_IRQCHIP) < 0) {
    perror("KVM_CREATE_IRQCHIP");
    exit(1);
}

// Now create vCPUs
int vcpu_fd = ioctl(vm_fd, KVM_CREATE_VCPU, 0);
```

**Benefits**:
- Lower latency for interrupts
- Better performance (no VM exits for interrupt delivery)
- Required for SMP guests

---

### 4.79 KVM_CREATE_DEVICE

**Purpose**: Create emulated device (e.g., VFIO, GICv3)

```c
Capability: KVM_CAP_DEVICE_CTRL
Type: vm ioctl
Parameters: struct kvm_create_device (in/out)
Returns: 0 on success, -1 on error
```

**Structure**:
```c
struct kvm_create_device {
    __u32 type;      /* KVM_DEV_TYPE_xxx */
    __u32 fd;        /* Output: device file descriptor */
    __u32 flags;     /* KVM_CREATE_DEVICE_TEST (test only, don't create) */
};
```

**Device Types**:
- `KVM_DEV_TYPE_VFIO`: VFIO device passthrough
- `KVM_DEV_TYPE_ARM_VGIC_V2`: ARM GICv2
- `KVM_DEV_TYPE_ARM_VGIC_V3`: ARM GICv3
- `KVM_DEV_TYPE_FSL_MPIC_20`: PowerPC MPIC
- And more...

**Example (ARM GICv3)**:
```c
struct kvm_create_device device = {
    .type = KVM_DEV_TYPE_ARM_VGIC_V3,
    .fd = -1,
    .flags = 0
};

if (ioctl(vm_fd, KVM_CREATE_DEVICE, &device) < 0) {
    perror("KVM_CREATE_DEVICE");
    exit(1);
}

int gic_fd = device.fd;
// Configure GIC via device ioctls on gic_fd
```

---

## vCPU ioctls

vCPU ioctls operate on vCPU file descriptors returned by `KVM_CREATE_VCPU`.

### 4.10 KVM_RUN

**Purpose**: Execute guest code on this vCPU

```c
Capability: basic
Architectures: all
Type: vcpu ioctl
Parameters: none (uses mmap'd region)
Returns: 0 on success, -1 on error
```

**The Execution Loop**:
```c
int vcpu_mmap_size = ioctl(kvm_fd, KVM_GET_VCPU_MMAP_SIZE, 0);
struct kvm_run *run = mmap(NULL, vcpu_mmap_size, PROT_READ | PROT_WRITE,
                           MAP_SHARED, vcpu_fd, 0);

// Main VM execution loop
while (1) {
    int ret = ioctl(vcpu_fd, KVM_RUN, 0);

    if (ret < 0) {
        if (errno == EINTR) continue;  // Signal received
        perror("KVM_RUN");
        break;
    }

    // Handle VM exit
    switch (run->exit_reason) {
        case KVM_EXIT_IO:
            handle_io(run);
            break;
        case KVM_EXIT_MMIO:
            handle_mmio(run);
            break;
        case KVM_EXIT_SHUTDOWN:
            printf("Guest requested shutdown\n");
            return 0;
        case KVM_EXIT_HLT:
            printf("Guest halted\n");
            break;
        default:
            printf("Unhandled exit reason: %d\n", run->exit_reason);
            return 1;
    }
}
```

**Exit Reasons**:

| Exit Reason | Description |
|-------------|-------------|
| `KVM_EXIT_IO` | Port I/O instruction |
| `KVM_EXIT_MMIO` | MMIO access |
| `KVM_EXIT_SHUTDOWN` | Triple fault or shutdown request |
| `KVM_EXIT_HLT` | HLT instruction |
| `KVM_EXIT_INTR` | Signal pending in userspace |
| `KVM_EXIT_FAIL_ENTRY` | Failed to enter guest mode |
| `KVM_EXIT_INTERNAL_ERROR` | Internal KVM error |
| `KVM_EXIT_SYSTEM_EVENT` | System event (shutdown/reset/crash) |

---

### 4.11 KVM_GET_REGS / 4.12 KVM_SET_REGS

**Purpose**: Get/set general-purpose registers

```c
Capability: basic
Architectures: all except ARM, arm64
Type: vcpu ioctl
Parameters: struct kvm_regs (out for GET, in for SET)
Returns: 0 on success, -1 on error
```

**x86 Structure**:
```c
struct kvm_regs {
    /* General purpose registers */
    __u64 rax, rbx, rcx, rdx;
    __u64 rsi, rdi, rsp, rbp;
    __u64 r8, r9, r10, r11;
    __u64 r12, r13, r14, r15;
    __u64 rip, rflags;
};
```

**Example**:
```c
struct kvm_regs regs;

// Get current register values
if (ioctl(vcpu_fd, KVM_GET_REGS, &regs) < 0) {
    perror("KVM_GET_REGS");
    exit(1);
}

printf("RIP: 0x%llx\n", regs.rip);
printf("RAX: 0x%llx\n", regs.rax);

// Set RIP to entry point
regs.rip = 0x100000;  // Guest entry point
regs.rflags = 0x2;    // Reserved bit must be 1

if (ioctl(vcpu_fd, KVM_SET_REGS, &regs) < 0) {
    perror("KVM_SET_REGS");
    exit(1);
}
```

---

### 4.13 KVM_GET_SREGS / 4.14 KVM_SET_SREGS

**Purpose**: Get/set special registers (segment registers, control registers)

```c
Capability: basic
Architectures: x86, ppc
Type: vcpu ioctl
Parameters: struct kvm_sregs (out for GET, in for SET)
Returns: 0 on success, -1 on error
```

**x86 Structure**:
```c
struct kvm_sregs {
    struct kvm_segment cs, ds, es, fs, gs, ss;  /* Segment registers */
    struct kvm_segment tr, ldt;                   /* Task/LDT registers */
    struct kvm_dtable gdt, idt;                   /* Descriptor tables */
    __u64 cr0, cr2, cr3, cr4, cr8;               /* Control registers */
    __u64 efer;                                   /* Extended feature enable */
    __u64 apic_base;                              /* APIC base address */
    __u64 interrupt_bitmap[(KVM_NR_INTERRUPTS + 63) / 64];
};
```

**Example (Setting up Protected Mode)**:
```c
struct kvm_sregs sregs;

if (ioctl(vcpu_fd, KVM_GET_SREGS, &sregs) < 0) {
    perror("KVM_GET_SREGS");
    exit(1);
}

// Enable protected mode
sregs.cr0 |= 1;  // PE bit

// Set up code segment
sregs.cs.base = 0;
sregs.cs.limit = 0xffffffff;
sregs.cs.selector = 0x8;  // Code segment selector
sregs.cs.type = 11;       // Execute/read, accessed
sregs.cs.present = 1;
sregs.cs.dpl = 0;         // Ring 0
sregs.cs.db = 1;          // 32-bit
sregs.cs.s = 1;           // Code/data segment
sregs.cs.l = 0;           // Not 64-bit
sregs.cs.g = 1;           // 4KB granularity

if (ioctl(vcpu_fd, KVM_SET_SREGS, &sregs) < 0) {
    perror("KVM_SET_SREGS");
    exit(1);
}
```

---

### 4.16 KVM_INTERRUPT

**Purpose**: Queue a hardware interrupt

```c
Capability: basic
Architectures: x86, ppc, mips
Type: vcpu ioctl
Parameters: struct kvm_interrupt (in)
Returns: 0 on success, negative on failure
```

**Structure**:
```c
struct kvm_interrupt {
    __u32 irq;  /* Interrupt vector number */
};
```

**Example**:
```c
struct kvm_interrupt irq = {
    .irq = 32  // Timer interrupt (IRQ 0)
};

if (ioctl(vcpu_fd, KVM_INTERRUPT, &irq) < 0) {
    if (errno == EEXIST) {
        // Interrupt already queued
    } else if (errno == EINVAL) {
        // Invalid IRQ number
    }
}
```

**Architecture Notes**:
- **x86**: IRQ is interrupt vector (not IRQ line)
- **PPC**: Special values for SIGP operations
- **MIPS**: Negative number dequeues interrupt

---

### 4.68 KVM_SET_ONE_REG / 4.69 KVM_GET_ONE_REG

**Purpose**: Get/set a single register by ID

```c
Capability: KVM_CAP_ONE_REG
Architectures: all
Type: vcpu ioctl
Parameters: struct kvm_one_reg (in for SET, in/out for GET)
Returns: 0 on success, negative on failure
```

**Structure**:
```c
struct kvm_one_reg {
    __u64 id;    /* Register identifier */
    __u64 addr;  /* Pointer to register value */
};
```

**Example (ARM64 general purpose register)**:
```c
__u64 x0_value;
struct kvm_one_reg reg = {
    .id = 0x6030000000100000,  // ARM64 X0
    .addr = (__u64)&x0_value
};

// Get X0
if (ioctl(vcpu_fd, KVM_GET_ONE_REG, &reg) < 0) {
    perror("KVM_GET_ONE_REG");
}
printf("X0 = 0x%llx\n", x0_value);

// Set X0
x0_value = 0x12345678;
if (ioctl(vcpu_fd, KVM_SET_ONE_REG, &reg) < 0) {
    perror("KVM_SET_ONE_REG");
}
```

**Register ID Encoding** (architecture-specific):
- ARM64: `0x60x0 0000 0010 <index>`
- PPC: See extensive register list in documentation
- MIPS: `0x70x0 0000 <type> <reg>`

---

## Memory Management

### 4.117 KVM_CLEAR_DIRTY_LOG

**Purpose**: Clear dirty page tracking bitmap

```c
Capability: KVM_CAP_MANUAL_DIRTY_LOG_PROTECT2
Architectures: x86, arm, arm64, mips
Type: vm ioctl
Parameters: struct kvm_clear_dirty_log (in)
Returns: 0 on success, -1 on error
```

**Structure**:
```c
struct kvm_clear_dirty_log {
    __u32 slot;                      /* Memory slot */
    __u32 num_pages;                 /* Number of pages to clear */
    __u64 first_page;                /* First page number */
    union {
        void __user *dirty_bitmap;   /* Pages to clear (one bit per page) */
        __u64 padding;
    };
};
```

**Example**:
```c
// Clear dirty bits for pages that have been migrated
unsigned long bitmap_size = (num_pages + 7) / 8;
unsigned char *bitmap = malloc(bitmap_size);
memset(bitmap, 0xff, bitmap_size);  // Mark all as needing clear

struct kvm_clear_dirty_log clear_log = {
    .slot = 0,
    .first_page = 0,
    .num_pages = num_pages,
    .dirty_bitmap = bitmap
};

if (ioctl(vm_fd, KVM_CLEAR_DIRTY_LOG, &clear_log) < 0) {
    perror("KVM_CLEAR_DIRTY_LOG");
}
```

**Benefits**:
- Fine-grained control over dirty tracking
- Better live migration performance
- Reduced overhead compared to `KVM_GET_DIRTY_LOG` auto-clear

---

## Interrupt Handling

### 4.52 KVM_SET_GSI_ROUTING

**Purpose**: Configure GSI (Global System Interrupt) routing

```c
Capability: KVM_CAP_IRQ_ROUTING
Architectures: x86, s390, arm, arm64
Type: vm ioctl
Parameters: struct kvm_irq_routing (in)
Returns: 0 on success, -1 on error
```

**Structure**:
```c
struct kvm_irq_routing {
    __u32 nr;                             /* Number of entries */
    __u32 flags;
    struct kvm_irq_routing_entry entries[0];
};

struct kvm_irq_routing_entry {
    __u32 gsi;       /* Global System Interrupt number */
    __u32 type;      /* KVM_IRQ_ROUTING_IRQCHIP, _MSI, etc. */
    __u32 flags;
    __u32 pad;
    union {
        struct kvm_irq_routing_irqchip irqchip;
        struct kvm_irq_routing_msi msi;
        struct kvm_irq_routing_s390_adapter adapter;
        struct kvm_irq_routing_hv_sint hv_sint;
        __u32 pad[8];
    } u;
};
```

**Example (Route GSI to IOAPIC)**:
```c
struct kvm_irq_routing *routing;
int nr_entries = 24;  // 0-23 for standard PC

routing = calloc(1, sizeof(*routing) + nr_entries * sizeof(routing->entries[0]));
routing->nr = nr_entries;
routing->flags = 0;

for (int i = 0; i < nr_entries; i++) {
    routing->entries[i].gsi = i;
    routing->entries[i].type = KVM_IRQ_ROUTING_IRQCHIP;
    routing->entries[i].u.irqchip.irqchip = 0;  // IOAPIC
    routing->entries[i].u.irqchip.pin = i;
}

if (ioctl(vm_fd, KVM_SET_GSI_ROUTING, routing) < 0) {
    perror("KVM_SET_GSI_ROUTING");
}
```

---

### 4.59 KVM_IOEVENTFD

**Purpose**: Register eventfd for fast I/O notification

```c
Capability: KVM_CAP_IOEVENTFD
Architectures: all
Type: vm ioctl
Parameters: struct kvm_ioeventfd (in)
Returns: 0 on success, negative on error
```

**Structure**:
```c
struct kvm_ioeventfd {
    __u64 datamatch;   /* Data value to match */
    __u64 addr;        /* I/O address (PIO/MMIO) */
    __u32 len;         /* Size: 0, 1, 2, 4, or 8 bytes */
    __s32 fd;          /* eventfd file descriptor */
    __u32 flags;
    __u8  pad[36];
};
```

**Flags**:
- `KVM_IOEVENTFD_FLAG_DATAMATCH`: Only trigger if data matches
- `KVM_IOEVENTFD_FLAG_PIO`: PIO address (else MMIO)
- `KVM_IOEVENTFD_FLAG_DEASSIGN`: Remove ioeventfd
- `KVM_IOEVENTFD_FLAG_VIRTIO_CCW_NOTIFY`: For virtio-ccw (s390)

**Example (Virtio device doorbell)**:
```c
int efd = eventfd(0, EFD_NONBLOCK);
if (efd < 0) {
    perror("eventfd");
    exit(1);
}

struct kvm_ioeventfd ioevent = {
    .addr = 0xd000,              // MMIO address for doorbell
    .len = 4,                     // 32-bit write
    .fd = efd,
    .flags = KVM_IOEVENTFD_FLAG_DATAMATCH,
    .datamatch = 0x1              // Only trigger on value 1
};

if (ioctl(vm_fd, KVM_IOEVENTFD, &ioevent) < 0) {
    perror("KVM_IOEVENTFD");
}

// In device emulation thread:
while (1) {
    uint64_t val;
    read(efd, &val, sizeof(val));
    // Process virtio queue
}
```

---

### 4.75 KVM_IRQFD

**Purpose**: Inject interrupts via eventfd (fast path)

```c
Capability: KVM_CAP_IRQFD
Architectures: x86, s390, arm, arm64
Type: vm ioctl
Parameters: struct kvm_irqfd (in)
Returns: 0 on success, -1 on error
```

**Structure**:
```c
struct kvm_irqfd {
    __u32 fd;          /* eventfd to trigger interrupt */
    __u32 gsi;         /* GSI number */
    __u32 flags;
    __u32 resamplefd;  /* eventfd for level-triggered resample */
    __u8  pad[16];
};
```

**Example**:
```c
int irq_efd = eventfd(0, EFD_NONBLOCK);

struct kvm_irqfd irqfd = {
    .fd = irq_efd,
    .gsi = 32,  // Interrupt vector
    .flags = 0
};

if (ioctl(vm_fd, KVM_IRQFD, &irqfd) < 0) {
    perror("KVM_IRQFD");
}

// Trigger interrupt from any thread
uint64_t val = 1;
write(irq_efd, &val, sizeof(val));  // Inject interrupt into guest
```

---

## Quick Reference Table

### System ioctls (on `/dev/kvm`)

| ioctl | Purpose | Returns |
|-------|---------|---------|
| `KVM_GET_API_VERSION` | Get API version | 12 |
| `KVM_CREATE_VM` | Create VM | VM FD |
| `KVM_CHECK_EXTENSION` | Check capability | 0 or feature level |
| `KVM_GET_VCPU_MMAP_SIZE` | Get kvm_run size | Size in bytes |
| `KVM_GET_SUPPORTED_CPUID` | Get supported CPUID | 0 on success |
| `KVM_GET_MSR_INDEX_LIST` | Get MSR list | 0 on success |

### VM ioctls (on VM FD)

| ioctl | Purpose | Returns |
|-------|---------|---------|
| `KVM_CREATE_VCPU` | Create vCPU | vCPU FD |
| `KVM_SET_USER_MEMORY_REGION` | Map memory | 0 on success |
| `KVM_CREATE_IRQCHIP` | Create interrupt controller | 0 on success |
| `KVM_GET_DIRTY_LOG` | Get dirty pages | 0 on success |
| `KVM_CLEAR_DIRTY_LOG` | Clear dirty tracking | 0 on success |
| `KVM_CREATE_DEVICE` | Create device | 0 on success |
| `KVM_SET_GSI_ROUTING` | Configure interrupts | 0 on success |
| `KVM_IOEVENTFD` | Register I/O eventfd | 0 on success |
| `KVM_IRQFD` | Register IRQ eventfd | 0 on success |

### vCPU ioctls (on vCPU FD)

| ioctl | Purpose | Returns |
|-------|---------|---------|
| `KVM_RUN` | Execute guest | 0 on exit |
| `KVM_GET_REGS` | Get GP registers | 0 on success |
| `KVM_SET_REGS` | Set GP registers | 0 on success |
| `KVM_GET_SREGS` | Get special registers | 0 on success |
| `KVM_SET_SREGS` | Set special registers | 0 on success |
| `KVM_GET_FPU` | Get FPU state | 0 on success |
| `KVM_SET_FPU` | Set FPU state | 0 on success |
| `KVM_INTERRUPT` | Queue interrupt | 0 on success |
| `KVM_GET_ONE_REG` | Get single register | 0 on success |
| `KVM_SET_ONE_REG` | Set single register | 0 on success |

---

## Common Usage Patterns

### Pattern 1: Basic VM Setup

```c
// 1. Open KVM
int kvm = open("/dev/kvm", O_RDWR);

// 2. Create VM
int vm = ioctl(kvm, KVM_CREATE_VM, 0);

// 3. Allocate memory
void *mem = mmap(NULL, 1<<30, PROT_READ|PROT_WRITE,
                 MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
struct kvm_userspace_memory_region region = {
    .slot = 0, .guest_phys_addr = 0,
    .memory_size = 1<<30, .userspace_addr = (unsigned long)mem
};
ioctl(vm, KVM_SET_USER_MEMORY_REGION, &region);

// 4. Create vCPU
int vcpu = ioctl(vm, KVM_CREATE_VCPU, 0);

// 5. Setup registers and run
// ... (set registers, load code, etc.)
ioctl(vcpu, KVM_RUN, 0);
```

### Pattern 2: Live Migration

```c
// 1. Enable dirty logging
region.flags = KVM_MEM_LOG_DIRTY_PAGES;
ioctl(vm, KVM_SET_USER_MEMORY_REGION, &region);

// 2. Iterative copy
while (dirty_pages > threshold) {
    ioctl(vm, KVM_GET_DIRTY_LOG, &log);
    transmit_dirty_pages(log.dirty_bitmap);
    ioctl(vm, KVM_CLEAR_DIRTY_LOG, &clear_log);
}

// 3. Stop VM and final copy
stop_vm();
ioctl(vm, KVM_GET_DIRTY_LOG, &log);
transmit_final_pages(log.dirty_bitmap);
```

### Pattern 3: Device Emulation with Eventfd

```c
// Setup ioeventfd for guest writes
int efd = eventfd(0, 0);
struct kvm_ioeventfd ioevent = {
    .addr = DEVICE_MMIO_ADDR,
    .len = 4,
    .fd = efd
};
ioctl(vm, KVM_IOEVENTFD, &ioevent);

// Setup irqfd for interrupts to guest
int irq_efd = eventfd(0, 0);
struct kvm_irqfd irqfd = {
    .fd = irq_efd,
    .gsi = DEVICE_IRQ
};
ioctl(vm, KVM_IRQFD, &irqfd);

// Device thread
while (1) {
    uint64_t val;
    read(efd, &val, 8);        // Guest wrote to device
    process_device_request();
    write(irq_efd, &val, 8);   // Signal completion
}
```

---

## Next Steps

- **[Practical Examples](06-practical-examples.md)**: Complete working code
- **[Memory Management Deep Dive](03-memory-management.md)**: EPT, huge pages, migration
- **[Performance Tuning](07-performance-tuning.md)**: Optimization techniques

---

**Last Updated**: 2025-11-15
**Covers**: KVM API version 12 (Linux 2.6.22+)
