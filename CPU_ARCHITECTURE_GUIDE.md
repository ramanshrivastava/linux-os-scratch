# CPU Architecture Deep Dive: Understanding How Processors Really Work

## Table of Contents
1. [CPU Fundamentals](#cpu-fundamentals)
2. [The Instruction Execution Pipeline](#the-instruction-execution-pipeline)
3. [CPU Cores Explained](#cpu-cores-explained)
4. [Process Management and Scheduling](#process-management-and-scheduling)
5. [Parallelism vs Concurrency](#parallelism-vs-concurrency)
6. [Modern CPU Features](#modern-cpu-features)

## CPU Fundamentals

### What is a CPU?
A CPU (Central Processing Unit) is the brain of a computer - a complex integrated circuit that executes program instructions. At its core, it's billions of transistors working together to perform calculations and make decisions.

### Basic CPU Architecture

```
┌──────────────────────────────────────────────────────────────────┐
│                         CPU DIE (Physical Chip)                  │
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                      CONTROL UNIT                          │ │
│  │  ┌──────────────┐  ┌─────────────┐  ┌────────────────┐  │ │
│  │  │ Instruction  │  │  Instruction │  │    Control     │  │ │
│  │  │   Fetcher    │→ │   Decoder    │→ │    Signals     │  │ │
│  │  └──────────────┘  └─────────────┘  └────────────────┘  │ │
│  └────────────────────────────────────────────────────────────┘ │
│                                ↓                                 │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                    EXECUTION CORE                          │ │
│  │  ┌─────────────────────────────────────────────────────┐  │ │
│  │  │              ALU (Arithmetic Logic Unit)            │  │ │
│  │  │  ┌──────────┐  ┌──────────┐  ┌──────────────────┐ │  │ │
│  │  │  │  Adder   │  │Multiplier│  │ Logical Ops     │ │  │ │
│  │  │  │ Circuit  │  │ Circuit  │  │ (AND/OR/XOR)    │ │  │ │
│  │  │  └──────────┘  └──────────┘  └──────────────────┘ │  │ │
│  │  └─────────────────────────────────────────────────────┘  │ │
│  │                                                            │ │
│  │  ┌─────────────────────────────────────────────────────┐  │ │
│  │  │                    FPU                              │  │ │
│  │  │         (Floating Point Unit)                       │  │ │
│  │  └─────────────────────────────────────────────────────┘  │ │
│  └────────────────────────────────────────────────────────────┘ │
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                      REGISTER FILE                         │ │
│  │  ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐│ │
│  │  │RAX │ │RBX │ │RCX │ │RDX │ │RSI │ │RDI │ │RBP │ │RSP ││ │
│  │  └────┘ └────┘ └────┘ └────┘ └────┘ └────┘ └────┘ └────┘│ │
│  │  ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐│ │
│  │  │R8  │ │R9  │ │R10 │ │R11 │ │R12 │ │R13 │ │R14 │ │R15 ││ │
│  │  └────┘ └────┘ └────┘ └────┘ └────┘ └────┘ └────┘ └────┘│ │
│  │  ┌────────────┐ ┌────────────┐ ┌──────────────────────┐  │ │
│  │  │    RIP     │ │   RFLAGS   │ │  Segment Registers  │  │ │
│  │  │ (Prog Ctr) │ │(Status Reg)│ │  (CS,DS,SS,ES,FS,GS)│  │ │
│  │  └────────────┘ └────────────┘ └──────────────────────┘  │ │
│  └────────────────────────────────────────────────────────────┘ │
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                      CACHE HIERARCHY                       │ │
│  │  ┌──────────────────────────────────────────────────────┐ │ │
│  │  │  L1 Cache (32KB Data + 32KB Instructions)            │ │ │
│  │  │  Access Time: 4 cycles (~1ns)                        │ │ │
│  │  └──────────────────────────────────────────────────────┘ │ │
│  │  ┌──────────────────────────────────────────────────────┐ │ │
│  │  │  L2 Cache (256KB - 1MB)                              │ │ │
│  │  │  Access Time: 12 cycles (~3ns)                       │ │ │
│  │  └──────────────────────────────────────────────────────┘ │ │
│  │  ┌──────────────────────────────────────────────────────┐ │ │
│  │  │  L3 Cache (8MB - 32MB) - Shared between cores        │ │ │
│  │  │  Access Time: 40 cycles (~10ns)                      │ │ │
│  │  └──────────────────────────────────────────────────────┘ │ │
│  └────────────────────────────────────────────────────────────┘ │
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │               MEMORY MANAGEMENT UNIT (MMU)                 │ │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐   │ │
│  │  │    TLB      │  │  Page Table │  │  Memory         │   │ │
│  │  │   Cache     │  │   Walker    │  │  Protection     │   │ │
│  │  └─────────────┘  └─────────────┘  └─────────────────┘   │ │
│  └────────────────────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────────────────────┘
                                ↕
                    ┌──────────────────────┐
                    │    System RAM         │
                    │  Access Time: 100ns   │
                    └──────────────────────┘
```

### Key Components Explained

1. **Control Unit**: The orchestrator that manages instruction flow
2. **ALU**: Performs arithmetic (+, -, *, /) and logical operations (AND, OR, NOT)
3. **Registers**: Ultra-fast storage inside the CPU (access in < 1 nanosecond)
4. **Cache**: Fast memory layers that store frequently used data
5. **MMU**: Translates virtual addresses to physical memory addresses

## The Instruction Execution Pipeline

Modern CPUs use pipelining to execute multiple instructions simultaneously at different stages:

```
┌──────────────────────────────────────────────────────────────────────┐
│                     5-STAGE INSTRUCTION PIPELINE                     │
├──────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  Clock Cycle:  1      2      3      4      5      6      7      8   │
│                                                                      │
│  Instruction 1: [IF]  [ID]  [EX]  [MEM] [WB]                       │
│  Instruction 2:       [IF]  [ID]  [EX]  [MEM] [WB]                 │
│  Instruction 3:             [IF]  [ID]  [EX]  [MEM] [WB]           │
│  Instruction 4:                   [IF]  [ID]  [EX]  [MEM] [WB]     │
│  Instruction 5:                         [IF]  [ID]  [EX]  [MEM] [WB]│
│                                                                      │
│  Stages:                                                            │
│  [IF] = Instruction Fetch    - Get instruction from memory          │
│  [ID] = Instruction Decode   - Determine what to do                 │
│  [EX] = Execute              - Perform the operation                │
│  [MEM] = Memory Access       - Read/write memory if needed          │
│  [WB] = Write Back           - Store result in register             │
│                                                                      │
└──────────────────────────────────────────────────────────────────────┘

Pipeline Hazards and Solutions:
┌──────────────────────────────────────────────────────────────────────┐
│  Data Hazard Example:                                                │
│  ADD R1, R2, R3    // R1 = R2 + R3                                  │
│  SUB R4, R1, R5    // R4 = R1 - R5 (needs R1 from previous instr)  │
│                                                                      │
│  Solution: Forwarding/Bypassing                                      │
│  ┌─────┐     ┌─────┐     ┌─────┐     ┌─────┐     ┌─────┐         │
│  │ IF  │ --> │ ID  │ --> │ EX  │ --> │ MEM │ --> │ WB  │         │
│  └─────┘     └─────┘     └──┬──┘     └─────┘     └─────┘         │
│                              │                                       │
│                              └──────── Forward result directly      │
│                                                ↓                     │
│  ┌─────┐     ┌─────┐     ┌─────┐     ┌─────┐     ┌─────┐         │
│  │ IF  │ --> │ ID  │ --> │ EX  │ --> │ MEM │ --> │ WB  │         │
│  └─────┘     └─────┘     └─────┘     └─────┘     └─────┘         │
└──────────────────────────────────────────────────────────────────────┘
```

### Superscalar Execution
Modern CPUs can execute multiple instructions per clock cycle:

```
┌──────────────────────────────────────────────────────────────────────┐
│                    SUPERSCALAR EXECUTION (4-wide)                    │
├──────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  Instruction Queue:                                                  │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐ ┌────────────┐│
│  │ADD R1,R2,R3  │ │SUB R4,R5,R6  │ │MUL R7,R8,R9  │ │XOR R10,R11 ││
│  └──────────────┘ └──────────────┘ └──────────────┘ └────────────┘│
│         ↓                ↓                ↓               ↓         │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐ ┌────────────┐│
│  │   ALU 1      │ │   ALU 2      │ │   MUL Unit   │ │  Logic Unit││
│  └──────────────┘ └──────────────┘ └──────────────┘ └────────────┘│
│                                                                      │
│  All four instructions execute simultaneously in one clock cycle!   │
└──────────────────────────────────────────────────────────────────────┘
```

## CPU Cores Explained

### Single Core vs Multi-Core Architecture

```
┌──────────────────────────────────────────────────────────────────────┐
│                        SINGLE-CORE CPU                               │
├──────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  Time →  T1    T2    T3    T4    T5    T6    T7    T8              │
│  Core 0: [P1]  [P2]  [P3]  [P1]  [P2]  [P3]  [P1]  [P2]            │
│          10ms  10ms  10ms  10ms  10ms  10ms  10ms  10ms            │
│                                                                      │
│  Processes P1, P2, P3 share the single core through time-slicing    │
└──────────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────────┐
│                        QUAD-CORE CPU                                 │
├──────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  Time →  T1    T2    T3    T4    T5    T6    T7    T8              │
│  Core 0: [P1]  [P1]  [P1]  [P1]  [P1]  [P1]  [P1]  [P1]            │
│  Core 1: [P2]  [P2]  [P2]  [P2]  [P2]  [P2]  [P2]  [P2]            │
│  Core 2: [P3]  [P3]  [P3]  [P3]  [P3]  [P3]  [P3]  [P3]            │
│  Core 3: [P4]  [P4]  [P4]  [P4]  [P4]  [P4]  [P4]  [P4]            │
│                                                                      │
│  Four processes run truly in parallel, each on its own core         │
└──────────────────────────────────────────────────────────────────────┘
```

### Modern Multi-Core CPU Architecture

```
┌────────────────────────────────────────────────────────────────────────┐
│                     MODERN 8-CORE CPU WITH SMT                         │
├────────────────────────────────────────────────────────────────────────┤
│                                                                        │
│  ┌─────────────────────────────┐  ┌─────────────────────────────┐    │
│  │      CORE COMPLEX 0          │  │      CORE COMPLEX 1          │    │
│  │  ┌───────┐  ┌───────┐       │  │  ┌───────┐  ┌───────┐       │    │
│  │  │Core 0 │  │Core 1 │       │  │  │Core 4 │  │Core 5 │       │    │
│  │  │Thread0│  │Thread0│       │  │  │Thread0│  │Thread0│       │    │
│  │  │Thread1│  │Thread1│       │  │  │Thread1│  │Thread1│       │    │
│  │  └───────┘  └───────┘       │  │  └───────┘  └───────┘       │    │
│  │  ┌───────┐  ┌───────┐       │  │  ┌───────┐  ┌───────┐       │    │
│  │  │Core 2 │  │Core 3 │       │  │  │Core 6 │  │Core 7 │       │    │
│  │  │Thread0│  │Thread0│       │  │  │Thread0│  │Thread0│       │    │
│  │  │Thread1│  │Thread1│       │  │  │Thread1│  │Thread1│       │    │
│  │  └───────┘  └───────┘       │  │  └───────┘  └───────┘       │    │
│  │  ┌─────────────────────┐    │  │  ┌─────────────────────┐    │    │
│  │  │  Shared L3 Cache    │    │  │  │  Shared L3 Cache    │    │    │
│  │  │      (16MB)         │    │  │  │      (16MB)         │    │    │
│  │  └─────────────────────┘    │  │  └─────────────────────┘    │    │
│  └─────────────────────────────┘  └─────────────────────────────┘    │
│                                                                        │
│  ┌────────────────────────────────────────────────────────────────┐   │
│  │                     SYSTEM AGENT                               │   │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────────┐   │   │
│  │  │   Memory     │  │    PCIe      │  │   Display        │   │   │
│  │  │  Controller  │  │  Controller  │  │   Controller     │   │   │
│  │  └──────────────┘  └──────────────┘  └──────────────────┘   │   │
│  └────────────────────────────────────────────────────────────────┘   │
│                                                                        │
│  Total: 8 Physical Cores, 16 Logical Cores (with Hyperthreading)      │
└────────────────────────────────────────────────────────────────────────┘
```

### How Hyperthreading (SMT) Works

```
┌──────────────────────────────────────────────────────────────────────┐
│              HYPERTHREADING - TWO THREADS, ONE CORE                  │
├──────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  ┌────────────────────────────────────────────────────────────┐     │
│  │                     PHYSICAL CORE                          │     │
│  │                                                            │     │
│  │  DUPLICATED (per thread):        SHARED:                  │     │
│  │  ┌──────────────────────┐       ┌──────────────────────┐ │     │
│  │  │   Thread 0           │       │                      │ │     │
│  │  │  ┌──────────────┐   │       │   ┌──────────────┐  │ │     │
│  │  │  │Architecture   │   │       │   │              │  │ │     │
│  │  │  │State (Regs)  │   │       │   │     ALU      │  │ │     │
│  │  │  └──────────────┘   │       │   │              │  │ │     │
│  │  │  ┌──────────────┐   │       │   └──────────────┘  │ │     │
│  │  │  │Program       │   │       │                      │ │     │
│  │  │  │Counter       │   │       │   ┌──────────────┐  │ │     │
│  │  │  └──────────────┘   │       │   │              │  │ │     │
│  │  └──────────────────────┘       │   │     FPU      │  │ │     │
│  │                                  │   │              │  │ │     │
│  │  ┌──────────────────────┐       │   └──────────────┘  │ │     │
│  │  │   Thread 1           │       │                      │ │     │
│  │  │  ┌──────────────┐   │       │   ┌──────────────┐  │ │     │
│  │  │  │Architecture   │   │       │   │   L1 Cache   │  │ │     │
│  │  │  │State (Regs)  │   │       │   │              │  │ │     │
│  │  │  └──────────────┘   │       │   └──────────────┘  │ │     │
│  │  │  ┌──────────────┐   │       │                      │ │     │
│  │  │  │Program       │   │       │   ┌──────────────┐  │ │     │
│  │  │  │Counter       │   │       │   │   L2 Cache   │  │ │     │
│  │  │  └──────────────┘   │       │   │              │  │ │     │
│  │  └──────────────────────┘       │   └──────────────┘  │ │     │
│  │                                  └──────────────────────┘ │     │
│  └────────────────────────────────────────────────────────────┘     │
│                                                                      │
│  Benefit: When Thread 0 waits for memory, Thread 1 can use ALU      │
└──────────────────────────────────────────────────────────────────────┘
```

## Process Management and Scheduling

### Process vs Thread
```
┌──────────────────────────────────────────────────────────────────────┐
│                      PROCESS vs THREAD                               │
├──────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  PROCESS (Heavy-weight)              THREAD (Light-weight)          │
│  ┌─────────────────────┐            ┌─────────────────────┐        │
│  │  Process A          │            │  Process B          │        │
│  │  ┌───────────────┐  │            │  ┌───────────────┐  │        │
│  │  │  Code Section │  │            │  │  Code Section │  │        │
│  │  └───────────────┘  │            │  │    (Shared)   │  │        │
│  │  ┌───────────────┐  │            │  └───────────────┘  │        │
│  │  │  Data Section │  │            │  ┌───────────────┐  │        │
│  │  └───────────────┘  │            │  │  Data Section │  │        │
│  │  ┌───────────────┐  │            │  │    (Shared)   │  │        │
│  │  │  Heap        │  │            │  └───────────────┘  │        │
│  │  └───────────────┘  │            │  ┌───────────────┐  │        │
│  │  ┌───────────────┐  │            │  │     Heap      │  │        │
│  │  │  Stack       │  │            │  │    (Shared)   │  │        │
│  │  └───────────────┘  │            │  └───────────────┘  │        │
│  │  ┌───────────────┐  │            │  ┌─────┬─────┬───┐  │        │
│  │  │  Registers   │  │            │  │Stack│Stack│...│  │        │
│  │  └───────────────┘  │            │  │ T1  │ T2  │   │  │        │
│  └─────────────────────┘            │  └─────┴─────┴───┘  │        │
│                                      │  ┌─────┬─────┬───┐  │        │
│  Isolated memory space               │  │Regs │Regs │...│  │        │
│  Context switch: ~1000ns             │  │ T1  │ T2  │   │  │        │
│                                      │  └─────┴─────┴───┘  │        │
│                                      └─────────────────────┘        │
│                                      Shared memory space            │
│                                      Context switch: ~100ns         │
└──────────────────────────────────────────────────────────────────────┘
```

### Context Switching Visualization

```
┌──────────────────────────────────────────────────────────────────────┐
│                     CONTEXT SWITCH OPERATION                         │
├──────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  Time: T0 (Process A Running)        Time: T1 (Switching)           │
│  ┌─────────────────┐                ┌─────────────────┐            │
│  │     CPU         │                │     CPU         │            │
│  │  ┌──────────┐   │                │  ┌──────────┐   │            │
│  │  │   RAX=5  │   │                │  │   RAX=5  │───┼──┐         │
│  │  │   RBX=10 │   │                │  │   RBX=10 │   │  │         │
│  │  │   RIP=100│   │                │  │   RIP=100│   │  │ Save    │
│  │  │   RSP=200│   │                │  │   RSP=200│   │  │ to PCB  │
│  │  └──────────┘   │                │  └──────────┘   │  │         │
│  └─────────────────┘                └─────────────────┘  │         │
│                                                           ↓         │
│                                      ┌─────────────────────┐        │
│                                      │  Process Control    │        │
│                                      │  Block (PCB) A      │        │
│  Time: T2 (Loading B)                │  RAX=5, RBX=10,    │        │
│  ┌─────────────────┐                │  RIP=100, RSP=200  │        │
│  │     CPU         │                └─────────────────────┘        │
│  │  ┌──────────┐   │←─────────────  ┌─────────────────────┐        │
│  │  │   RAX=42 │   │   Load from    │  Process Control    │        │
│  │  │   RBX=7  │   │   PCB B        │  Block (PCB) B      │        │
│  │  │   RIP=500│   │                │  RAX=42, RBX=7,     │        │
│  │  │   RSP=600│   │                │  RIP=500, RSP=600  │        │
│  │  └──────────┘   │                └─────────────────────┘        │
│  └─────────────────┘                                               │
│  Process B Running                                                  │
│                                                                      │
│  Total Context Switch Time: ~1-10 microseconds                      │
└──────────────────────────────────────────────────────────────────────┘
```

### CPU Scheduler States and Transitions

```
┌──────────────────────────────────────────────────────────────────────┐
│                    PROCESS STATE DIAGRAM                             │
├──────────────────────────────────────────────────────────────────────┤
│                                                                      │
│     ┌─────────┐                                                     │
│     │   NEW   │  Process is being created                           │
│     └────┬────┘                                                     │
│          │ Admitted                                                  │
│          ↓                                                          │
│     ┌─────────┐  Scheduler ────────→ ┌─────────┐                   │
│     │  READY  │  Dispatch            │ RUNNING │                   │
│     └────┬────┘ ←──────────────────  └────┬────┘                   │
│          ↑      Interrupt/Time slice       │                        │
│          │                                 │                        │
│          │                                 │ I/O or event wait      │
│          │                                 ↓                        │
│          │      I/O or event      ┌─────────────┐                   │
│          └──────  completion ←────│   WAITING   │                   │
│                                   └─────────────┘                   │
│                                           │                         │
│                                           │ Exit                    │
│                                           ↓                         │
│                                   ┌─────────────┐                   │
│                                   │ TERMINATED  │                   │
│                                   └─────────────┘                   │
└──────────────────────────────────────────────────────────────────────┘
```

## Parallelism vs Concurrency

### Visual Comparison

```
┌──────────────────────────────────────────────────────────────────────┐
│                  CONCURRENCY vs PARALLELISM                          │
├──────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  CONCURRENCY (Single Core)          PARALLELISM (Multi Core)        │
│  ─────────────────────────          ──────────────────────         │
│                                                                      │
│  Time →                              Time →                         │
│  Core 0: █░█░█░█░█░█░               Core 0: ████████████           │
│          A B A B A B                        AAAAAAAAAA             │
│                                      Core 1: ████████████           │
│  Legend:                                     BBBBBBBBBB             │
│  █ = Process A                      Core 2: ████████████           │
│  ░ = Process B                               CCCCCCCCCC             │
│                                      Core 3: ████████████           │
│  Interleaved execution                       DDDDDDDDDD             │
│  (Rapid switching)                                                  │
│                                      True simultaneous execution     │
│                                                                      │
│  Restaurant Analogy:                Restaurant Analogy:             │
│  1 chef, 4 dishes                   4 chefs, 4 dishes              │
│  Chef switches between dishes       Each chef cooks one dish        │
│  All dishes progress, but           All dishes cook at same time    │
│  only one at a time                                                 │
└──────────────────────────────────────────────────────────────────────┘
```

### Real-World Scheduling Example

```
┌──────────────────────────────────────────────────────────────────────┐
│              PROCESS SCHEDULING ON 4-CORE SYSTEM                     │
├──────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  System Load: 10 Processes (P0-P9)                                  │
│  CPU Cores: 4                                                       │
│  Time Quantum: 10ms                                                 │
│                                                                      │
│  Run Queue:                                                         │
│  ┌────┬────┬────┬────┬────┬────┬────┬────┬────┬────┐              │
│  │ P0 │ P1 │ P2 │ P3 │ P4 │ P5 │ P6 │ P7 │ P8 │ P9 │              │
│  └────┴────┴────┴────┴────┴────┴────┴────┴────┴────┘              │
│                                                                      │
│  Time:    0-10ms        10-20ms       20-30ms       30-40ms        │
│  ────────────────────────────────────────────────────────────      │
│  Core 0:  [  P0  ]      [  P4  ]      [  P8  ]      [  P0  ]       │
│  Core 1:  [  P1  ]      [  P5  ]      [  P9  ]      [  P1  ]       │
│  Core 2:  [  P2  ]      [  P6  ]      [  P0  ]      [  P2  ]       │
│  Core 3:  [  P3  ]      [  P7  ]      [  P1  ]      [  P3  ]       │
│                                                                      │
│  Waiting: P4,P5,P6,     P8,P9,P0,     P2,P3,P4,     P4,P5,P6,      │
│           P7,P8,P9      P1,P2,P3      P5,P6,P7      P7,P8,P9       │
│                                                                      │
│  Each process gets CPU time, but max 4 run truly in parallel        │
└──────────────────────────────────────────────────────────────────────┘
```

## Modern CPU Features

### Branch Prediction

```
┌──────────────────────────────────────────────────────────────────────┐
│                      BRANCH PREDICTION                               │
├──────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  Code Example:                      Branch History Table:           │
│  for(i=0; i<1000; i++) {           ┌──────────────────────┐        │
│    if(i < 999) {      ←─────────── │ Address │ Prediction │        │
│      sum += array[i];              │ 0x1000  │   TAKEN    │        │
│    }                                │ Pattern: TTTTTTTT...T│        │
│  }                                  └──────────────────────┘        │
│                                                                      │
│  Without Prediction:                With Prediction:                │
│  ┌────────────────┐                ┌────────────────┐              │
│  │   Fetch IF     │                │   Fetch IF     │              │
│  └────────┬───────┘                └────────┬───────┘              │
│           ↓                                 ↓                       │
│  ┌────────────────┐                ┌────────────────┐              │
│  │  Decode BRANCH │                │  Decode BRANCH │              │
│  └────────┬───────┘                └────────┬───────┘              │
│           ↓                                 ↓                       │
│  ┌────────────────┐                ┌────────────────┐              │
│  │    STALL!      │                │ Fetch Next (PR)│ ← Predicted  │
│  │  Wait for      │                └────────┬───────┘              │
│  │  branch result │                         ↓                       │
│  └────────┬───────┘                ┌────────────────┐              │
│           ↓                        │   Continue     │              │
│  ┌────────────────┐                │   Execution    │              │
│  │  Fetch Next    │                └────────────────┘              │
│  └────────────────┘                                                │
│                                                                      │
│  15-20 cycle penalty                No penalty if correct           │
└──────────────────────────────────────────────────────────────────────┘
```

### Out-of-Order Execution

```
┌──────────────────────────────────────────────────────────────────────┐
│                    OUT-OF-ORDER EXECUTION                            │
├──────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  Original Program Order:            Execution Order (Optimized):    │
│  1. LOAD  R1, [memory]   (slow)    1. ADD  R3, R4, R5  ← Execute   │
│  2. ADD   R2, R1, #5               2. SUB  R6, R7, R8  ← these     │
│  3. ADD   R3, R4, R5               3. MUL  R9, R10,R11 ← while     │
│  4. SUB   R6, R7, R8               4. LOAD R1, [memory] ← waiting  │
│  5. MUL   R9, R10, R11             5. ADD  R2, R1, #5  ← for load │
│                                                                      │
│  ┌──────────────────────────────────────────────────────────┐      │
│  │              RESERVATION STATION                          │      │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │      │
│  │  │ Instruction 1│  │ Instruction 3│  │ Instruction 4│  │      │
│  │  │   Waiting    │  │    Ready     │  │    Ready     │  │      │
│  │  └──────────────┘  └──────┬───────┘  └──────┬───────┘  │      │
│  │                            ↓                 ↓           │      │
│  │                     ┌──────────────┐  ┌──────────────┐  │      │
│  │                     │   ALU 1      │  │   ALU 2      │  │      │
│  │                     └──────────────┘  └──────────────┘  │      │
│  └──────────────────────────────────────────────────────────┘      │
│                                                                      │
│  ROB (Reorder Buffer) ensures results commit in program order       │
└──────────────────────────────────────────────────────────────────────┘
```

### SIMD (Single Instruction, Multiple Data)

```
┌──────────────────────────────────────────────────────────────────────┐
│                          SIMD OPERATIONS                             │
├──────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  Traditional (Scalar):              SIMD (Vector):                  │
│                                                                      │
│  for(i=0; i<4; i++)                 Single Instruction:             │
│    C[i] = A[i] + B[i]               VADD YMM0, YMM1, YMM2          │
│                                                                      │
│  4 iterations:                      1 operation:                    │
│  ┌────┐ ┌────┐   ┌────┐           ┌────┬────┬────┬────┐           │
│  │ A0 │+│ B0 │ = │ C0 │           │ A0 │ A1 │ A2 │ A3 │           │
│  └────┘ └────┘   └────┘           └────┴────┴────┴────┘           │
│  ┌────┐ ┌────┐   ┌────┐                    +                       │
│  │ A1 │+│ B1 │ = │ C1 │           ┌────┬────┬────┬────┐           │
│  └────┘ └────┘   └────┘           │ B0 │ B1 │ B2 │ B3 │           │
│  ┌────┐ ┌────┐   ┌────┐           └────┴────┴────┴────┘           │
│  │ A2 │+│ B2 │ = │ C2 │                    =                       │
│  └────┘ └────┘   └────┘           ┌────┬────┬────┬────┐           │
│  ┌────┐ ┌────┐   ┌────┐           │ C0 │ C1 │ C2 │ C3 │           │
│  │ A3 │+│ B3 │ = │ C3 │           └────┴────┴────┴────┘           │
│  └────┘ └────┘   └────┘           All 4 additions in 1 cycle!     │
│                                                                      │
│  4 clock cycles                    1 clock cycle                    │
│                                    4x speedup!                       │
└──────────────────────────────────────────────────────────────────────┘
```

## Performance Metrics and Optimization

### CPU Performance Hierarchy

```
┌──────────────────────────────────────────────────────────────────────┐
│                    MEMORY ACCESS LATENCY                             │
├──────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  Component          Latency        Bandwidth      Size              │
│  ─────────────────────────────────────────────────────────────     │
│  CPU Registers      < 1 ns         Extreme        ~1 KB            │
│       ↓                                                             │
│  L1 Cache          ~1 ns          Very High      32-64 KB         │
│       ↓                                                             │
│  L2 Cache          ~3 ns          High           256KB-1MB        │
│       ↓                                                             │
│  L3 Cache          ~10 ns         Medium         8-32 MB          │
│       ↓                                                             │
│  Main RAM          ~100 ns        Low            8-64 GB          │
│       ↓                                                             │
│  SSD               ~100 μs        Very Low       256GB-4TB        │
│       ↓                                                             │
│  HDD               ~10 ms         Extremely Low  1-20 TB          │
│                                                                      │
│  Analogy: If L1 cache = 1 second                                   │
│           Then RAM = 1.5 minutes                                   │
│           And HDD = 4 months!                                      │
└──────────────────────────────────────────────────────────────────────┘
```

## Summary

The CPU is an incredibly complex piece of engineering that:
- Executes billions of instructions per second
- Uses multiple cores for true parallel processing
- Employs sophisticated techniques like pipelining, prediction, and caching
- Manages hundreds of processes through rapid context switching
- Provides the illusion of multitasking even on single-core systems

Understanding these concepts is crucial for:
- Writing efficient code
- Debugging performance issues
- System programming and OS development
- Optimizing applications for modern hardware

The key insight: While a single core can only execute one instruction stream at a time, modern CPUs use multiple cores, hyperthreading, and rapid context switching to handle many processes efficiently, creating a seamless multitasking experience.