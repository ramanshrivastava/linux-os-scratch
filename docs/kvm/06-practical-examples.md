# KVM Practical Examples and Code

This document provides complete, working code examples for using the KVM API.

## Table of Contents

1. [Minimal VM Example](#minimal-vm-example)
2. [Real Mode VM with BIOS](#real-mode-vm-with-bios)
3. [Protected Mode VM](#protected-mode-vm)
4. [Multi-vCPU VM](#multi-vcpu-vm)
5. [Device Emulation](#device-emulation)
6. [Live Migration](#live-migration)
7. [Building and Running](#building-and-running)

---

## Minimal VM Example

This is the simplest possible KVM program that creates a VM and executes a few instructions.

### Code: `minimal_kvm.c`

```c
/*
 * minimal_kvm.c - Minimal KVM example
 *
 * Creates a VM that executes: mov $0x42, %eax; hlt
 * Demonstrates basic KVM API usage.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/kvm.h>
#include <errno.h>

int main(void) {
    int kvm_fd, vm_fd, vcpu_fd;
    int vcpu_mmap_size;
    struct kvm_run *run;
    struct kvm_userspace_memory_region region;
    struct kvm_regs regs;
    struct kvm_sregs sregs;
    void *mem;

    // Guest code: mov $0x42, %eax; hlt
    // Machine code: B8 42 00 00 00 F4
    uint8_t code[] = {
        0xb8, 0x42, 0x00, 0x00, 0x00,  // mov $0x42, %eax
        0xf4                            // hlt
    };

    printf("Opening /dev/kvm...\n");
    kvm_fd = open("/dev/kvm", O_RDWR | O_CLOEXEC);
    if (kvm_fd < 0) {
        perror("open /dev/kvm");
        return 1;
    }

    // Check KVM API version
    int api_version = ioctl(kvm_fd, KVM_GET_API_VERSION, 0);
    if (api_version != 12) {
        fprintf(stderr, "KVM API version %d, expected 12\n", api_version);
        return 1;
    }
    printf("KVM API version: %d\n", api_version);

    // Create VM
    printf("Creating VM...\n");
    vm_fd = ioctl(kvm_fd, KVM_CREATE_VM, 0);
    if (vm_fd < 0) {
        perror("KVM_CREATE_VM");
        return 1;
    }

    // Allocate guest memory (4KB)
    printf("Allocating guest memory...\n");
    mem = mmap(NULL, 0x1000, PROT_READ | PROT_WRITE,
               MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    // Copy guest code to memory
    memcpy(mem, code, sizeof(code));

    // Map guest memory
    region.slot = 0;
    region.flags = 0;
    region.guest_phys_addr = 0x0;
    region.memory_size = 0x1000;
    region.userspace_addr = (unsigned long)mem;

    if (ioctl(vm_fd, KVM_SET_USER_MEMORY_REGION, &region) < 0) {
        perror("KVM_SET_USER_MEMORY_REGION");
        return 1;
    }
    printf("Guest memory mapped at GPA 0x0, size 4KB\n");

    // Create vCPU
    printf("Creating vCPU...\n");
    vcpu_fd = ioctl(vm_fd, KVM_CREATE_VCPU, 0);
    if (vcpu_fd < 0) {
        perror("KVM_CREATE_VCPU");
        return 1;
    }

    // Map kvm_run structure
    vcpu_mmap_size = ioctl(kvm_fd, KVM_GET_VCPU_MMAP_SIZE, 0);
    if (vcpu_mmap_size < 0) {
        perror("KVM_GET_VCPU_MMAP_SIZE");
        return 1;
    }

    run = mmap(NULL, vcpu_mmap_size, PROT_READ | PROT_WRITE,
               MAP_SHARED, vcpu_fd, 0);
    if (run == MAP_FAILED) {
        perror("mmap kvm_run");
        return 1;
    }

    // Get current special registers
    if (ioctl(vcpu_fd, KVM_GET_SREGS, &sregs) < 0) {
        perror("KVM_GET_SREGS");
        return 1;
    }

    // Set up code segment (real mode)
    sregs.cs.base = 0;
    sregs.cs.selector = 0;

    if (ioctl(vcpu_fd, KVM_SET_SREGS, &sregs) < 0) {
        perror("KVM_SET_SREGS");
        return 1;
    }

    // Set instruction pointer to start of code
    memset(&regs, 0, sizeof(regs));
    regs.rip = 0;
    regs.rflags = 0x2;  // Reserved bit

    if (ioctl(vcpu_fd, KVM_SET_REGS, &regs) < 0) {
        perror("KVM_SET_REGS");
        return 1;
    }

    // Run the VM
    printf("Running VM...\n");
    if (ioctl(vcpu_fd, KVM_RUN, 0) < 0) {
        perror("KVM_RUN");
        return 1;
    }

    // Check exit reason
    printf("VM exited with reason: %d\n", run->exit_reason);

    if (run->exit_reason == KVM_EXIT_HLT) {
        printf("VM halted successfully!\n");

        // Get final register state
        if (ioctl(vcpu_fd, KVM_GET_REGS, &regs) < 0) {
            perror("KVM_GET_REGS");
            return 1;
        }

        printf("Final EAX value: 0x%llx\n", regs.rax);

        if (regs.rax == 0x42) {
            printf("SUCCESS: Guest executed correctly!\n");
        } else {
            printf("ERROR: Expected EAX=0x42, got 0x%llx\n", regs.rax);
        }
    }

    // Cleanup
    munmap(run, vcpu_mmap_size);
    munmap(mem, 0x1000);
    close(vcpu_fd);
    close(vm_fd);
    close(kvm_fd);

    return 0;
}
```

### Expected Output

```
Opening /dev/kvm...
KVM API version: 12
Creating VM...
Allocating guest memory...
Guest memory mapped at GPA 0x0, size 4KB
Creating vCPU...
Running VM...
VM exited with reason: 5
VM halted successfully!
Final EAX value: 0x42
SUCCESS: Guest executed correctly!
```

---

## Real Mode VM with BIOS

This example shows how to create a VM that runs in real mode, similar to how a PC starts up.

### Code: `real_mode_vm.c`

```c
/*
 * real_mode_vm.c - Real mode VM with simple output
 *
 * Creates a 16-bit real mode VM that writes "Hello" to port 0xE9
 * (QEMU debug console port)
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/kvm.h>

#define RAM_SIZE (1 << 20)  // 1MB

int main(void) {
    int kvm_fd, vm_fd, vcpu_fd, vcpu_mmap_size;
    struct kvm_run *run;
    struct kvm_userspace_memory_region region;
    struct kvm_regs regs;
    struct kvm_sregs sregs;
    void *mem;

    /*
     * Guest code (16-bit real mode):
     * Print "Hello" to port 0xE9
     * Each character is output via: mov $char, %al; out %al, $0xE9
     */
    uint8_t code[] = {
        // Print 'H'
        0xb0, 'H',           // mov $'H', %al
        0xe6, 0xe9,          // out %al, $0xE9
        // Print 'e'
        0xb0, 'e',           // mov $'e', %al
        0xe6, 0xe9,          // out %al, $0xE9
        // Print 'l'
        0xb0, 'l',           // mov $'l', %al
        0xe6, 0xe9,          // out %al, $0xE9
        // Print 'l'
        0xb0, 'l',           // mov $'l', %al
        0xe6, 0xe9,          // out %al, $0xE9
        // Print 'o'
        0xb0, 'o',           // mov $'o', %al
        0xe6, 0xe9,          // out %al, $0xE9
        // Print '\n'
        0xb0, '\n',          // mov $'\n', %al
        0xe6, 0xe9,          // out %al, $0xE9
        // Halt
        0xf4                 // hlt
    };

    // Open KVM
    kvm_fd = open("/dev/kvm", O_RDWR | O_CLOEXEC);
    if (kvm_fd < 0) {
        perror("open /dev/kvm");
        return 1;
    }

    // Create VM
    vm_fd = ioctl(kvm_fd, KVM_CREATE_VM, 0);
    if (vm_fd < 0) {
        perror("KVM_CREATE_VM");
        return 1;
    }

    // Allocate guest memory
    mem = mmap(NULL, RAM_SIZE, PROT_READ | PROT_WRITE,
               MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) {
        perror("mmap");
        return 1;
    }
    memset(mem, 0, RAM_SIZE);

    // Load code at address 0x0
    memcpy(mem, code, sizeof(code));

    // Map guest memory
    region.slot = 0;
    region.flags = 0;
    region.guest_phys_addr = 0x0;
    region.memory_size = RAM_SIZE;
    region.userspace_addr = (unsigned long)mem;

    if (ioctl(vm_fd, KVM_SET_USER_MEMORY_REGION, &region) < 0) {
        perror("KVM_SET_USER_MEMORY_REGION");
        return 1;
    }

    // Create vCPU
    vcpu_fd = ioctl(vm_fd, KVM_CREATE_VCPU, 0);
    if (vcpu_fd < 0) {
        perror("KVM_CREATE_VCPU");
        return 1;
    }

    // Map kvm_run
    vcpu_mmap_size = ioctl(kvm_fd, KVM_GET_VCPU_MMAP_SIZE, 0);
    run = mmap(NULL, vcpu_mmap_size, PROT_READ | PROT_WRITE,
               MAP_SHARED, vcpu_fd, 0);
    if (run == MAP_FAILED) {
        perror("mmap kvm_run");
        return 1;
    }

    // Setup special registers (real mode)
    if (ioctl(vcpu_fd, KVM_GET_SREGS, &sregs) < 0) {
        perror("KVM_GET_SREGS");
        return 1;
    }

    sregs.cs.base = 0;
    sregs.cs.selector = 0;

    if (ioctl(vcpu_fd, KVM_SET_SREGS, &sregs) < 0) {
        perror("KVM_SET_SREGS");
        return 1;
    }

    // Setup general purpose registers
    memset(&regs, 0, sizeof(regs));
    regs.rip = 0;
    regs.rflags = 0x2;

    if (ioctl(vcpu_fd, KVM_SET_REGS, &regs) < 0) {
        perror("KVM_SET_REGS");
        return 1;
    }

    printf("VM starting...\n");
    printf("Guest output: ");
    fflush(stdout);

    // VM execution loop
    while (1) {
        if (ioctl(vcpu_fd, KVM_RUN, 0) < 0) {
            perror("KVM_RUN");
            return 1;
        }

        switch (run->exit_reason) {
        case KVM_EXIT_HLT:
            printf("\nVM halted\n");
            goto done;

        case KVM_EXIT_IO:
            if (run->io.direction == KVM_EXIT_IO_OUT &&
                run->io.port == 0xE9) {
                // Debug console output
                char *data = (char *)run + run->io.data_offset;
                for (uint32_t i = 0; i < run->io.size; i++) {
                    putchar(data[i]);
                }
                fflush(stdout);
            } else {
                fprintf(stderr, "Unexpected I/O: port=0x%x dir=%s\n",
                        run->io.port,
                        run->io.direction == KVM_EXIT_IO_IN ? "in" : "out");
            }
            break;

        case KVM_EXIT_FAIL_ENTRY:
            fprintf(stderr, "KVM_EXIT_FAIL_ENTRY: hardware_entry_failure_reason = 0x%llx\n",
                    run->fail_entry.hardware_entry_failure_reason);
            return 1;

        case KVM_EXIT_INTERNAL_ERROR:
            fprintf(stderr, "KVM_EXIT_INTERNAL_ERROR: suberror = 0x%x\n",
                    run->internal.suberror);
            return 1;

        default:
            fprintf(stderr, "Unhandled exit reason: %d\n", run->exit_reason);
            return 1;
        }
    }

done:
    // Cleanup
    munmap(run, vcpu_mmap_size);
    munmap(mem, RAM_SIZE);
    close(vcpu_fd);
    close(vm_fd);
    close(kvm_fd);

    return 0;
}
```

### Expected Output

```
VM starting...
Guest output: Hello
VM halted
```

---

## Protected Mode VM

This example demonstrates setting up a VM in 32-bit protected mode with a GDT.

### Code: `protected_mode_vm.c`

```c
/*
 * protected_mode_vm.c - Protected mode VM example
 *
 * Sets up a VM in 32-bit protected mode with proper GDT
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/kvm.h>

#define RAM_SIZE (64 << 20)  // 64MB
#define CODE_START 0x100000   // 1MB

// GDT entry structure
struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed));

void setup_gdt(void *mem) {
    struct gdt_entry *gdt = (struct gdt_entry *)(mem + 0x800);

    // Null descriptor (entry 0)
    memset(&gdt[0], 0, sizeof(struct gdt_entry));

    // Code segment (entry 1, selector 0x08)
    gdt[1].limit_low = 0xFFFF;
    gdt[1].base_low = 0;
    gdt[1].base_mid = 0;
    gdt[1].access = 0x9A;      // Present, Ring 0, Code, Execute/Read
    gdt[1].granularity = 0xCF; // 4KB granularity, 32-bit
    gdt[1].base_high = 0;

    // Data segment (entry 2, selector 0x10)
    gdt[2].limit_low = 0xFFFF;
    gdt[2].base_low = 0;
    gdt[2].base_mid = 0;
    gdt[2].access = 0x92;      // Present, Ring 0, Data, Read/Write
    gdt[2].granularity = 0xCF; // 4KB granularity, 32-bit
    gdt[2].base_high = 0;
}

void setup_segment(struct kvm_segment *seg, uint32_t base, uint32_t limit,
                   uint8_t type, uint8_t dpl) {
    seg->base = base;
    seg->limit = limit;
    seg->selector = 0;
    seg->type = type;
    seg->present = 1;
    seg->dpl = dpl;
    seg->db = 1;       // 32-bit
    seg->s = 1;        // Code/data segment
    seg->l = 0;        // Not 64-bit
    seg->g = 1;        // 4KB granularity
    seg->avl = 0;
}

int main(void) {
    int kvm_fd, vm_fd, vcpu_fd, vcpu_mmap_size;
    struct kvm_run *run;
    struct kvm_userspace_memory_region region;
    struct kvm_regs regs;
    struct kvm_sregs sregs;
    void *mem;

    /*
     * Protected mode code:
     * mov $0x12345678, %eax
     * mov %eax, 0xB8000   (write to VGA text buffer)
     * hlt
     */
    uint8_t code[] = {
        0xb8, 0x78, 0x56, 0x34, 0x12,  // mov $0x12345678, %eax
        0xa3, 0x00, 0x80, 0x0b, 0x00,  // mov %eax, 0xB8000
        0xf4                            // hlt
    };

    // Open KVM and create VM
    kvm_fd = open("/dev/kvm", O_RDWR | O_CLOEXEC);
    if (kvm_fd < 0) {
        perror("open /dev/kvm");
        return 1;
    }

    vm_fd = ioctl(kvm_fd, KVM_CREATE_VM, 0);
    if (vm_fd < 0) {
        perror("KVM_CREATE_VM");
        return 1;
    }

    // Allocate guest memory
    mem = mmap(NULL, RAM_SIZE, PROT_READ | PROT_WRITE,
               MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) {
        perror("mmap");
        return 1;
    }
    memset(mem, 0, RAM_SIZE);

    // Setup GDT
    setup_gdt(mem);

    // Load code
    memcpy(mem + CODE_START, code, sizeof(code));

    // Map guest memory
    region.slot = 0;
    region.flags = 0;
    region.guest_phys_addr = 0;
    region.memory_size = RAM_SIZE;
    region.userspace_addr = (unsigned long)mem;

    if (ioctl(vm_fd, KVM_SET_USER_MEMORY_REGION, &region) < 0) {
        perror("KVM_SET_USER_MEMORY_REGION");
        return 1;
    }

    // Create vCPU
    vcpu_fd = ioctl(vm_fd, KVM_CREATE_VCPU, 0);
    if (vcpu_fd < 0) {
        perror("KVM_CREATE_VCPU");
        return 1;
    }

    vcpu_mmap_size = ioctl(kvm_fd, KVM_GET_VCPU_MMAP_SIZE, 0);
    run = mmap(NULL, vcpu_mmap_size, PROT_READ | PROT_WRITE,
               MAP_SHARED, vcpu_fd, 0);
    if (run == MAP_FAILED) {
        perror("mmap kvm_run");
        return 1;
    }

    // Get special registers
    if (ioctl(vcpu_fd, KVM_GET_SREGS, &sregs) < 0) {
        perror("KVM_GET_SREGS");
        return 1;
    }

    // Enable protected mode
    sregs.cr0 |= 1;  // Set PE bit

    // Setup GDT
    sregs.gdt.base = 0x800;
    sregs.gdt.limit = 3 * sizeof(struct gdt_entry) - 1;

    // Setup code segment (selector 0x08)
    setup_segment(&sregs.cs, 0, 0xFFFFFFFF, 11, 0);
    sregs.cs.selector = 0x08;

    // Setup data segments (selector 0x10)
    setup_segment(&sregs.ds, 0, 0xFFFFFFFF, 3, 0);
    sregs.ds.selector = 0x10;
    setup_segment(&sregs.es, 0, 0xFFFFFFFF, 3, 0);
    sregs.es.selector = 0x10;
    setup_segment(&sregs.fs, 0, 0xFFFFFFFF, 3, 0);
    sregs.fs.selector = 0x10;
    setup_segment(&sregs.gs, 0, 0xFFFFFFFF, 3, 0);
    sregs.gs.selector = 0x10;
    setup_segment(&sregs.ss, 0, 0xFFFFFFFF, 3, 0);
    sregs.ss.selector = 0x10;

    if (ioctl(vcpu_fd, KVM_SET_SREGS, &sregs) < 0) {
        perror("KVM_SET_SREGS");
        return 1;
    }

    // Setup general registers
    memset(&regs, 0, sizeof(regs));
    regs.rip = CODE_START;
    regs.rflags = 0x2;

    if (ioctl(vcpu_fd, KVM_SET_REGS, &regs) < 0) {
        perror("KVM_SET_REGS");
        return 1;
    }

    printf("Running protected mode VM...\n");

    // Run VM
    if (ioctl(vcpu_fd, KVM_RUN, 0) < 0) {
        perror("KVM_RUN");
        return 1;
    }

    if (run->exit_reason == KVM_EXIT_HLT) {
        printf("VM halted successfully\n");

        // Check if value was written to VGA buffer
        uint32_t *vga = (uint32_t *)(mem + 0xB8000);
        printf("VGA buffer value: 0x%08x\n", *vga);

        if (*vga == 0x12345678) {
            printf("SUCCESS: Protected mode execution worked!\n");
        }
    } else {
        printf("Unexpected exit reason: %d\n", run->exit_reason);
    }

    // Cleanup
    munmap(run, vcpu_mmap_size);
    munmap(mem, RAM_SIZE);
    close(vcpu_fd);
    close(vm_fd);
    close(kvm_fd);

    return 0;
}
```

---

## Building and Running

### Makefile

```makefile
CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = -static

EXAMPLES = minimal_kvm real_mode_vm protected_mode_vm

all: $(EXAMPLES)

minimal_kvm: minimal_kvm.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

real_mode_vm: real_mode_vm.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

protected_mode_vm: protected_mode_vm.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(EXAMPLES)

test: all
	@echo "Running minimal_kvm..."
	sudo ./minimal_kvm
	@echo
	@echo "Running real_mode_vm..."
	sudo ./real_mode_vm
	@echo
	@echo "Running protected_mode_vm..."
	sudo ./protected_mode_vm

.PHONY: all clean test
```

### Building

```bash
# Build all examples
make

# Build specific example
make minimal_kvm

# Run all tests
make test
```

### Requirements

- Linux kernel with KVM support
- `/dev/kvm` device accessible
- Root privileges or user in `kvm` group

```bash
# Check if KVM is available
ls -l /dev/kvm

# Add user to kvm group (Ubuntu/Debian)
sudo usermod -aG kvm $USER

# Check if CPU supports virtualization
egrep -o '(vmx|svm)' /proc/cpuinfo
```

---

## Debugging Tips

### Enable KVM Tracing

```bash
# Enable KVM tracing
sudo trace-cmd record -e kvm ./your_program

# View trace
trace-cmd report
```

### GDB Debugging

```bash
# Debug guest code
gdb ./your_program

# Set breakpoint before KVM_RUN
(gdb) break ioctl
(gdb) run

# Examine guest memory
(gdb) x/10i mem_address
```

### Common Issues

1. **Permission Denied on /dev/kvm**
   ```bash
   sudo chmod 666 /dev/kvm
   # Or add user to kvm group
   ```

2. **KVM Module Not Loaded**
   ```bash
   sudo modprobe kvm
   sudo modprobe kvm_intel  # or kvm_amd
   ```

3. **Guest Doesn't Execute**
   - Check register initialization
   - Verify memory is correctly mapped
   - Check guest code is at correct address

---

## Next Steps

- **[API Reference](02-kvm-api-reference.md)**: Complete ioctl documentation
- **[Architecture Deep Dive](01-kvm-architecture.md)**: Understanding KVM internals
- **[Performance Tuning](07-performance-tuning.md)**: Optimization techniques

---

**Last Updated**: 2025-11-15
