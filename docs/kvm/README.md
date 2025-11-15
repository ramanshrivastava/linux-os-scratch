# KVM (Kernel-based Virtual Machine) Documentation

## Table of Contents

1. [Introduction to KVM](#introduction-to-kvm)
2. [How KVM Works](#how-kvm-works)
3. [Connection to Linux](#connection-to-linux)
4. [Connection to Cloud-Based VMs](#connection-to-cloud-based-vms)
5. [KVM API Overview](#kvm-api-overview)
6. [Documentation Structure](#documentation-structure)

---

## Introduction to KVM

**KVM (Kernel-based Virtual Machine)** is a virtualization infrastructure built into the Linux kernel that transforms Linux into a Type-1 (bare-metal) hypervisor. It allows multiple virtual machines (VMs) to run on a single physical host, each with its own virtualized hardware.

### Key Characteristics

- **Kernel Module**: KVM is implemented as a loadable kernel module (`kvm.ko`)
- **Hardware-Assisted**: Requires CPU virtualization extensions (Intel VT-x or AMD-V)
- **Full Virtualization**: Provides complete hardware virtualization
- **Performance**: Near-native performance through hardware acceleration
- **Open Source**: Part of the mainline Linux kernel since version 2.6.20 (2007)

### Why KVM Matters

KVM powers much of the modern cloud infrastructure:
- **Amazon EC2**: Uses KVM as its primary hypervisor
- **Google Compute Engine**: Built on KVM
- **OpenStack**: Default hypervisor for cloud deployments
- **Red Hat Virtualization**: Enterprise virtualization based on KVM
- **Proxmox VE**: Open-source virtualization platform using KVM

---

## How KVM Works

### Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                    User Space Applications                   │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐         │
│  │   QEMU/     │  │  libvirt    │  │   virsh     │         │
│  │   KVM VMM   │  │             │  │             │         │
│  └─────────────┘  └─────────────┘  └─────────────┘         │
└─────────────────────────────────────────────────────────────┘
                          ↕ (ioctl system calls)
┌─────────────────────────────────────────────────────────────┐
│                       Linux Kernel                           │
│  ┌──────────────────────────────────────────────────────┐   │
│  │              KVM Kernel Module                       │   │
│  │  ┌────────────┐  ┌────────────┐  ┌────────────┐    │   │
│  │  │  /dev/kvm  │  │  VM ioctls │  │ vCPU ioctls│    │   │
│  │  └────────────┘  └────────────┘  └────────────┘    │   │
│  └──────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
                          ↕ (CPU extensions)
┌─────────────────────────────────────────────────────────────┐
│                      Physical Hardware                       │
│  ┌────────────┐  ┌────────────┐  ┌────────────┐           │
│  │  CPU with  │  │   Memory   │  │    I/O     │           │
│  │  VT-x/AMD-V│  │            │  │  Devices   │           │
│  └────────────┘  └────────────┘  └────────────┘           │
└─────────────────────────────────────────────────────────────┘
```

### Component Breakdown

#### 1. **KVM Kernel Module** (`/dev/kvm`)
- Provides the `/dev/kvm` device file
- Handles CPU virtualization (Intel VT-x or AMD-V)
- Manages VM and vCPU (virtual CPU) structures
- Handles memory management (EPT/NPT)
- Intercepts privileged instructions from guest VMs

#### 2. **QEMU (Quick Emulator)**
- **Device Emulation**: Emulates hardware devices (disk, network, graphics)
- **I/O Handling**: Manages I/O operations for VMs
- **VM Management**: Creates and manages VM instances
- **User Interface**: Provides command-line and monitor interfaces

#### 3. **libvirt**
- **Management Layer**: Provides unified API for managing VMs
- **XML Configuration**: VM definitions in XML format
- **Network/Storage**: Manages virtual networks and storage pools
- **Remote Management**: Enables remote VM management

### KVM Execution Flow

1. **VM Creation**: User space (QEMU) opens `/dev/kvm` and creates a VM via `KVM_CREATE_VM` ioctl
2. **vCPU Setup**: Creates virtual CPUs with `KVM_CREATE_VCPU`
3. **Memory Mapping**: Maps guest physical memory to host virtual memory via `KVM_SET_USER_MEMORY_REGION`
4. **Guest Execution**:
   - QEMU calls `KVM_RUN` ioctl
   - KVM switches CPU to guest mode
   - Guest code executes directly on physical CPU
   - On VM exit (I/O, interrupt, etc.), control returns to QEMU
5. **Event Handling**: QEMU handles the exit reason and resumes guest

---

## Connection to Linux

### KVM as Part of Linux Kernel

KVM is not a separate hypervisor—it **transforms Linux itself into a hypervisor**:

```
Traditional Hypervisor Model:
┌──────────────────┐
│   Guest OS 1     │
├──────────────────┤
│   Guest OS 2     │
├──────────────────┤
│   Hypervisor     │ ← Separate layer
├──────────────────┤
│   Hardware       │
└──────────────────┘

KVM Model:
┌──────────────────┐
│   Guest OS 1     │ ← VM as Linux process
├──────────────────┤
│   Guest OS 2     │ ← VM as Linux process
├──────────────────┤
│ Linux Kernel     │ ← Kernel IS the hypervisor
│  + KVM module    │
├──────────────────┤
│   Hardware       │
└──────────────────┘
```

### Integration Points

#### 1. **Process Model**
- Each VM runs as a regular Linux process (QEMU process)
- Each vCPU is a thread in that process
- Benefits from Linux scheduler, memory management, I/O stack

#### 2. **Memory Management**
- Uses Linux page tables
- Leverages huge pages (2MB, 1GB) for performance
- Integrates with NUMA (Non-Uniform Memory Access)
- Memory overcommitment through KSM (Kernel Same-page Merging)

#### 3. **Device Drivers**
- **Virtio**: Paravirtualized I/O framework
- **VFIO**: Direct device assignment (GPU, network cards)
- **TAP/TUN**: Virtual network interfaces
- Leverages existing Linux drivers

#### 4. **Security**
- SELinux/AppArmor for VM isolation
- seccomp for syscall filtering
- Namespaces and cgroups for resource isolation

#### 5. **Performance Features**
- KSM (Kernel Same-page Merging): Deduplicates memory pages
- Transparent Huge Pages: Automatic large page support
- CPU pinning and NUMA awareness
- Real-time kernel support for low-latency VMs

### Linux Kernel Subsystems Used by KVM

```
┌─────────────────────────────────────────────────────────┐
│                    KVM Module                            │
│                                                          │
│  Uses:                                                   │
│  ├─ Memory Management (mm/)                             │
│  ├─ Scheduler (kernel/sched/)                           │
│  ├─ Interrupt Handling (kernel/irq/)                    │
│  ├─ CPU Management (arch/x86/kvm/)                      │
│  ├─ Device Model (drivers/vfio/)                        │
│  └─ File Systems (for disk images)                      │
└─────────────────────────────────────────────────────────┘
```

---

## Connection to Cloud-Based VMs

### KVM in Cloud Infrastructure

KVM is the foundation of most modern Infrastructure-as-a-Service (IaaS) platforms. Here's how it powers cloud VMs:

### 1. **Public Cloud Providers**

#### Amazon Web Services (AWS)
- **Nitro System**: Custom KVM-based hypervisor
- **EC2 Instances**: Run on KVM (post-2017)
- **Customizations**:
  - Custom boot process (Nitro security chip)
  - SR-IOV for network performance
  - Enhanced CPU features

#### Google Cloud Platform (GCP)
- **Compute Engine**: Built entirely on KVM
- **Custom Kernel**: Optimized Linux kernel with KVM
- **Live Migration**: Seamless VM migration across hosts

#### Microsoft Azure
- Initially Hyper-V, now includes KVM for Linux workloads
- Azure Stack uses KVM for certain scenarios

#### IBM Cloud, DigitalOcean, Linode
- All use KVM as primary hypervisor

### 2. **Private Cloud Platforms**

#### OpenStack
```
OpenStack Architecture with KVM:

┌───────────────────────────────────────────────────────┐
│              OpenStack Control Plane                   │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌─────────┐ │
│  │  Nova    │ │ Neutron  │ │  Cinder  │ │ Glance  │ │
│  │(Compute) │ │(Network) │ │(Storage) │ │ (Images)│ │
│  └──────────┘ └──────────┘ └──────────┘ └─────────┘ │
└───────────────────────────────────────────────────────┘
                      ↓ (APIs)
┌───────────────────────────────────────────────────────┐
│              Compute Nodes (Linux + KVM)              │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐              │
│  │  VM 1   │  │  VM 2   │  │  VM 3   │              │
│  └─────────┘  └─────────┘  └─────────┘              │
│              libvirt + QEMU-KVM                       │
└───────────────────────────────────────────────────────┘
```

- **Nova**: Compute service uses libvirt to manage KVM VMs
- **Live Migration**: Move VMs between hosts without downtime
- **Resource Scheduling**: Intelligent VM placement

#### oVirt / Red Hat Virtualization
- Enterprise virtualization management for KVM
- Web-based management interface
- High availability and disaster recovery

#### Proxmox VE
- Open-source virtualization platform
- Web interface for KVM management
- Container support alongside VMs

### 3. **Cloud VM Lifecycle with KVM**

```
User Request → Cloud API → Management Layer → KVM Host
                                                    ↓
┌──────────────────────────────────────────────────────────┐
│ 1. VM Creation                                           │
│    ├─ API receives request (e.g., AWS EC2 RunInstances) │
│    ├─ Scheduler selects physical host                   │
│    ├─ Image retrieved from object storage                │
│    └─ KVM_CREATE_VM ioctl called on /dev/kvm            │
├──────────────────────────────────────────────────────────┤
│ 2. Resource Allocation                                   │
│    ├─ Memory: KVM_SET_USER_MEMORY_REGION                │
│    ├─ CPUs: KVM_CREATE_VCPU (multiple vCPUs)            │
│    ├─ Disk: Attach virtual disk (qcow2/raw)             │
│    └─ Network: Create TAP interface + assign IP         │
├──────────────────────────────────────────────────────────┤
│ 3. VM Execution                                          │
│    ├─ QEMU process starts                                │
│    ├─ Guest OS boots                                     │
│    ├─ KVM_RUN executes guest code                       │
│    └─ User applications run inside VM                   │
├──────────────────────────────────────────────────────────┤
│ 4. Operations                                            │
│    ├─ Snapshot: Create point-in-time copy               │
│    ├─ Resize: Add/remove vCPUs, memory                  │
│    ├─ Migrate: Move to different host                   │
│    └─ Monitor: Collect metrics (CPU, memory, I/O)       │
├──────────────────────────────────────────────────────────┤
│ 5. VM Termination                                        │
│    ├─ Graceful shutdown or force stop                   │
│    ├─ Resources released                                 │
│    └─ Billing stops                                      │
└──────────────────────────────────────────────────────────┘
```

### 4. **Cloud-Specific KVM Optimizations**

#### Nested Virtualization
- Run VMs inside VMs (containers in VMs)
- Used by Kubernetes on cloud VMs

#### SR-IOV (Single Root I/O Virtualization)
- Direct hardware access for VMs
- Near-native network performance
- Used in AWS Enhanced Networking

#### CPU Pinning
- Dedicate physical CPU cores to VMs
- Reduces latency and improves performance
- Used for high-performance computing instances

#### Live Migration
- Move running VMs between hosts
- Zero downtime during maintenance
- Essential for cloud SLA guarantees

#### Overcommitment
- Allocate more resources than physically available
- Memory ballooning and KSM
- Reduces infrastructure costs

### 5. **Cloud Storage Integration**

```
VM Disk Options in Cloud:

┌─────────────────────────────────────────────────┐
│ 1. Local Disk (Ephemeral)                       │
│    └─ Direct QEMU disk image on host           │
├─────────────────────────────────────────────────┤
│ 2. Network Block Storage (Persistent)          │
│    ├─ AWS EBS: iSCSI/NVMe over network         │
│    ├─ Ceph RBD: Distributed block storage      │
│    └─ GCP Persistent Disks                     │
├─────────────────────────────────────────────────┤
│ 3. Shared File Systems                          │
│    ├─ NFS/CIFS mounts                           │
│    └─ Cluster file systems (GlusterFS)         │
└─────────────────────────────────────────────────┘
```

### 6. **Security in Cloud KVM**

```
Security Layers:

┌─────────────────────────────────────────────────┐
│ Cloud Tenant 1      Cloud Tenant 2              │
│  ┌─────────┐         ┌─────────┐               │
│  │  VM A   │         │  VM B   │               │
│  └─────────┘         └─────────┘               │
├─────────────────────────────────────────────────┤
│ KVM Isolation                                   │
│  ├─ Memory isolation (EPT/NPT)                 │
│  ├─ CPU isolation (separate processes)         │
│  └─ I/O isolation (virtio, VFIO)               │
├─────────────────────────────────────────────────┤
│ Linux Security                                  │
│  ├─ SELinux/AppArmor                           │
│  ├─ Namespaces (PID, network, mount)          │
│  ├─ cgroups (resource limits)                  │
│  └─ seccomp (syscall filtering)                │
├─────────────────────────────────────────────────┤
│ Hardware Security                               │
│  ├─ CPU features (SMEP, SMAP)                  │
│  ├─ IOMMU (VT-d/AMD-Vi)                        │
│  └─ Secure Boot / TPM                          │
└─────────────────────────────────────────────────┘
```

---

## KVM API Overview

The KVM API is exposed through a set of **ioctl** system calls on file descriptors:

### Three-Level Hierarchy

```
1. System Level (/dev/kvm)
   ↓ KVM_CREATE_VM
2. VM Level (VM file descriptor)
   ↓ KVM_CREATE_VCPU
3. vCPU Level (vCPU file descriptor)
   ↓ KVM_RUN (execute guest code)
```

### Key API Categories

| Category | Description | Example ioctls |
|----------|-------------|----------------|
| **System** | Query capabilities, create VMs | `KVM_GET_API_VERSION`, `KVM_CREATE_VM` |
| **VM Management** | Configure VM-wide settings | `KVM_SET_USER_MEMORY_REGION`, `KVM_CREATE_VCPU` |
| **vCPU Control** | Control virtual CPU execution | `KVM_RUN`, `KVM_GET_REGS`, `KVM_SET_REGS` |
| **Memory** | Manage guest physical memory | `KVM_SET_USER_MEMORY_REGION`, `KVM_GET_DIRTY_LOG` |
| **Interrupts** | Inject interrupts into guest | `KVM_INTERRUPT`, `KVM_CREATE_IRQCHIP` |
| **Device Emulation** | Emulate hardware devices | `KVM_IOEVENTFD`, `KVM_IRQFD` |

### Simple Example Flow

```c
// 1. Open KVM device
int kvm_fd = open("/dev/kvm", O_RDWR);

// 2. Check API version
int version = ioctl(kvm_fd, KVM_GET_API_VERSION, 0);

// 3. Create VM
int vm_fd = ioctl(kvm_fd, KVM_CREATE_VM, 0);

// 4. Allocate guest memory
struct kvm_userspace_memory_region region = {
    .slot = 0,
    .guest_phys_addr = 0,
    .memory_size = 1024 * 1024 * 512, // 512MB
    .userspace_addr = (unsigned long)malloc(512 * 1024 * 1024)
};
ioctl(vm_fd, KVM_SET_USER_MEMORY_REGION, &region);

// 5. Create vCPU
int vcpu_fd = ioctl(vm_fd, KVM_CREATE_VCPU, 0);

// 6. Run vCPU
ioctl(vcpu_fd, KVM_RUN, 0);
```

---

## Documentation Structure

This documentation is organized into the following files:

```
docs/kvm/
├── README.md                          # This file - Overview and concepts
├── 01-kvm-architecture.md            # Deep dive into KVM architecture
├── 02-kvm-api-reference.md           # Complete API ioctl reference
├── 03-memory-management.md           # Memory virtualization details
├── 04-cpu-virtualization.md          # vCPU management and execution
├── 05-device-emulation.md            # I/O and device emulation
├── 06-practical-examples.md          # Code examples and tutorials
├── 07-performance-tuning.md          # Optimization techniques
└── 08-cloud-integration.md           # Cloud platform integration
```

### Quick Start Guides

- **[For Developers](06-practical-examples.md)**: Build your first KVM-based VMM
- **[For Cloud Engineers](08-cloud-integration.md)**: Deploy and manage KVM in production
- **[For System Programmers](02-kvm-api-reference.md)**: Complete API reference

---

## Key Takeaways

1. **KVM is Linux**: It's not a separate hypervisor but transforms Linux into one
2. **Hardware Acceleration**: Requires and leverages CPU virtualization extensions
3. **Cloud Foundation**: Powers AWS, GCP, and most cloud infrastructure
4. **API-Driven**: Controlled via ioctl system calls on `/dev/kvm`
5. **Performance**: Near-native performance through direct CPU execution
6. **Ecosystem**: Works with QEMU, libvirt, and cloud management platforms

---

## Next Steps

1. **Understand the Architecture**: Read [KVM Architecture Deep Dive](01-kvm-architecture.md)
2. **Explore the API**: See [KVM API Reference](02-kvm-api-reference.md)
3. **Try Examples**: Work through [Practical Examples](06-practical-examples.md)
4. **Learn Cloud Integration**: Study [Cloud Integration Guide](08-cloud-integration.md)

---

**Last Updated**: 2025-11-15
**KVM Version Covered**: Linux Kernel 5.x - 6.x
**API Version**: 12 (stable since Linux 2.6.22)
