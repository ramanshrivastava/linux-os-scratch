# Understanding Bits, Bytes, and Registers

## The Foundation: Bits and Bytes

### What is a Bit?
A **bit** (binary digit) is the smallest unit of data in a computer. It can only be:
- `0` (off, false, no voltage)
- `1` (on, true, voltage present)

Think of it like a light switch - either ON or OFF.

### What is a Byte?
A **byte** is 8 bits grouped together. Why 8? Historical reasons and it's enough to represent:
- 256 different values (2^8 = 256)
- Any ASCII character
- Numbers from 0-255 (unsigned) or -128 to 127 (signed)

```
1 byte = 8 bits
Example: 01000001 = 65 in decimal = 'A' in ASCII
         ||||||||
         |||||||└─ bit 0 (LSB - Least Significant Bit)
         ||||||└── bit 1
         |||||└─── bit 2
         ||||└──── bit 3
         |||└───── bit 4
         ||└────── bit 5
         |└─────── bit 6
         └──────── bit 7 (MSB - Most Significant Bit)
```

### Larger Units
```
1 byte     = 8 bits
1 word     = 2 bytes = 16 bits  (in x86 architecture)
1 dword    = 4 bytes = 32 bits  (double word)
1 qword    = 8 bytes = 64 bits  (quad word)

1 KB = 1,024 bytes
1 MB = 1,024 KB = 1,048,576 bytes
1 GB = 1,024 MB = 1,073,741,824 bytes
```

## Number Systems

### Binary (Base 2)
Uses only 0 and 1
```
10110010 (binary) = 178 (decimal)
Calculation: 1×128 + 0×64 + 1×32 + 1×16 + 0×8 + 0×4 + 1×2 + 0×1 = 178
```

### Hexadecimal (Base 16)
Uses 0-9 and A-F (where A=10, B=11, C=12, D=13, E=14, F=15)
```
0xB2 (hex) = 178 (decimal) = 10110010 (binary)

Why hex? It's more readable:
- Each hex digit represents exactly 4 bits
- B = 1011, 2 = 0010
- So B2 = 10110010
```

### Converting Between Systems
```
Decimal 65:
- Binary:  01000001
- Hex:     0x41
- ASCII:   'A'

In our boot sector:
0xAA55 (hex) = 10101010 01010101 (binary) = 43605 (decimal)
But stored as 55 AA in memory (little-endian)
```

## CPU Registers - The CPU's Variables

Registers are small, ultra-fast storage locations directly inside the CPU. Think of them as the CPU's local variables.

### Register Sizes Through History

```
8-bit registers  (8086/8088):   AL, AH, BL, BH, CL, CH, DL, DH
16-bit registers (8086-80286):  AX, BX, CX, DX, SI, DI, BP, SP
32-bit registers (80386+):      EAX, EBX, ECX, EDX, ESI, EDI, EBP, ESP
64-bit registers (x86_64):      RAX, RBX, RCX, RDX, RSI, RDI, RBP, RSP
                                R8-R15 (new in 64-bit)
```

### Register Anatomy - The RAX Family

```
64-bit: RAX (full register)
        ┌─────────────────────────────────────────────────────────┐
        │                            RAX                          │
        └─────────────────────────────────────────────────────────┘
        63                                                        0

32-bit: EAX (lower 32 bits of RAX)
                                ┌─────────────────────────────────┐
                                │              EAX                │
                                └─────────────────────────────────┘
                                31                                0

16-bit: AX (lower 16 bits of EAX)
                                                ┌─────────────────┐
                                                │       AX        │
                                                └─────────────────┘
                                                15                0

8-bit: AH (high byte of AX) and AL (low byte of AX)
                                                ┌────────┬────────┐
                                                │   AH   │   AL   │
                                                └────────┴────────┘
                                                15      8 7       0
```

### Real Example from Our Boot Sector

```asm
xor ax, ax      ; AX = 0
```
This sets the 16-bit AX register to 0. Here's what happens:

```
Before: AX might be 0x1234
        AH = 0x12, AL = 0x34
        Binary: 0001 0010 0011 0100

XOR operation (exclusive OR):
        0x1234 XOR 0x1234 = 0x0000
        Because: any number XOR itself = 0

After:  AX = 0x0000
        AH = 0x00, AL = 0x00
        Binary: 0000 0000 0000 0000
```

### General Purpose Registers and Their Common Uses

#### Data Registers (can be split into high/low bytes)
```
RAX/EAX/AX/AH/AL - Accumulator
  - Return values from functions
  - Multiplication/division operations
  - General arithmetic

RBX/EBX/BX/BH/BL - Base
  - Base pointer for memory access
  - General purpose storage

RCX/ECX/CX/CH/CL - Counter
  - Loop counters
  - Shift/rotate counts
  - String operations count

RDX/EDX/DX/DH/DL - Data
  - I/O port access
  - High 32/64 bits in multiplication
  - Remainder in division
```

#### Index and Pointer Registers (cannot be split)
```
RSI/ESI/SI - Source Index
  - Source pointer for string operations
  - General pointer operations

RDI/EDI/DI - Destination Index
  - Destination pointer for string operations
  - General pointer operations

RBP/EBP/BP - Base Pointer
  - Stack frame base pointer
  - Points to current stack frame

RSP/ESP/SP - Stack Pointer
  - Points to top of stack
  - Automatically adjusted by PUSH/POP
```

#### Instruction Pointer
```
RIP/EIP/IP - Instruction Pointer
  - Points to next instruction to execute
  - Cannot be directly modified
  - Changed by JMP, CALL, RET instructions
```

#### Segment Registers (16-bit only)
```
CS - Code Segment
  - Points to segment containing code

DS - Data Segment
  - Points to segment containing data

SS - Stack Segment
  - Points to segment containing stack

ES, FS, GS - Extra Segments
  - Additional data segment pointers
```

## Real Examples from Our Boot Sector

### Example 1: Setting Up Segments
```asm
xor ax, ax      ; AX = 0x0000
mov ds, ax      ; DS = 0x0000
```

Step by step:
1. `xor ax, ax`: Sets all 16 bits of AX to 0
2. `mov ds, ax`: Copies the value in AX (0) to DS

Why? In real mode, physical address = (segment × 16) + offset
So DS=0 means our data starts at physical address 0x0000

### Example 2: Stack Setup
```asm
mov ss, ax      ; Stack Segment = 0
mov sp, 0x7C00  ; Stack Pointer = 0x7C00
```

The stack grows downward, so we set SP to 0x7C00 (where our boot sector is loaded).
When we PUSH a value:
1. SP is decremented by 2 (in 16-bit mode)
2. Value is stored at the new SP location

### Example 3: BIOS Interrupt Call
```asm
mov ah, 0x0E    ; Function number in high byte of AX
mov al, 'A'     ; Character in low byte of AX
int 0x10        ; Call BIOS video interrupt
```

This uses the AX register split into two parts:
- AH (bits 8-15): Function number 0x0E (teletype output)
- AL (bits 0-7): Character to print 'A' (0x41)

Together: AX = 0x0E41

### Example 4: Memory Addressing
```asm
mov si, boot_msg    ; SI points to our message
lodsb              ; Load byte from [DS:SI] into AL, increment SI
```

Here's what happens with `lodsb`:
1. Read byte from memory address DS:SI
2. Store it in AL (lower 8 bits of AX)
3. Increment SI by 1 (point to next byte)

If boot_msg is at offset 0x7C50 and DS=0:
- Physical address = (0 × 16) + 0x7C50 = 0x7C50
- First byte loaded into AL
- SI becomes 0x7C51

## Bit Manipulation Operations

### AND - Clears specific bits
```
mov al, 0b11110000
and al, 0b11001100
Result: 0b11000000  (only bits that are 1 in BOTH)
```

### OR - Sets specific bits
```
mov al, 0b11110000
or  al, 0b00001111
Result: 0b11111111  (bits that are 1 in EITHER)
```

### XOR - Toggles bits
```
mov al, 0b11110000
xor al, 0b11111111
Result: 0b00001111  (flips all bits)
```

### NOT - Inverts all bits
```
mov al, 0b11110000
not al
Result: 0b00001111
```

### Shift Operations
```
mov al, 0b00000100  ; Decimal 4
shl al, 1          ; Shift left by 1
Result: 0b00001000  ; Decimal 8 (multiply by 2)

mov al, 0b00001000  ; Decimal 8
shr al, 1          ; Shift right by 1
Result: 0b00000100  ; Decimal 4 (divide by 2)
```

## The Boot Signature Explained

Our boot sector ends with:
```asm
dw 0xAA55    ; Boot signature
```

This writes the 16-bit value 0xAA55, but in little-endian format:
- First byte (offset 510): 0x55
- Second byte (offset 511): 0xAA

In binary:
```
Offset 510: 01010101  (0x55)
Offset 511: 10101010  (0xAA)
```

Why this specific pattern?
- It's unlikely to occur randomly
- Has alternating bits (good for testing)
- Easy to recognize in hex dumps
- Historical standard from IBM PC

## Memory and Registers Interaction

When we load our boot sector:
```
Physical Memory:
0x07C00: [Our boot sector code - 512 bytes]
         [...]
0x07DFE: 55 AA  (boot signature)

Registers after BIOS loads us:
DL = Boot drive number (0x00 for floppy, 0x80 for hard disk)
CS:IP = 0000:7C00 (execution starts here)
```

## Practice: Decoding Instructions

Let's decode `mov ax, 0x07E0`:
```
Instruction bytes: B8 E0 07

B8 = mov ax, immediate16
E0 07 = 0x07E0 (little-endian, so stored as E0 07)

When executed:
- AX register = 0x07E0
- AH = 0x07, AL = 0xE0
```

## Key Points to Remember

1. **Registers are FAST** - Much faster than memory access
2. **Size matters** - Using AL vs AX vs EAX vs RAX affects how many bits are modified
3. **Some instructions use specific registers** - MUL always uses AX/DX, LOOP uses CX
4. **Stack grows DOWN** - PUSH decrements SP, POP increments SP
5. **Little-endian storage** - Least significant byte stored first in memory
6. **Segment:Offset addressing** - In real mode, physical = segment×16 + offset

## Testing Your Understanding

Can you answer these?

1. If AX = 0x1234, what are the values of AH and AL?
   - Answer: AH = 0x12, AL = 0x34

2. What's the result of: mov al, 0xFF; inc al?
   - Answer: AL = 0x00 (overflow wraps around)

3. If SP = 0x7C00 and you PUSH AX (16-bit), what's the new SP?
   - Answer: SP = 0x7BFE (decreased by 2)

4. How many different values can a 16-bit register hold?
   - Answer: 65,536 (2^16 = 65,536, from 0 to 65,535)

This foundation is critical for understanding how our OS manipulates hardware directly!