# Boot Sector Analysis - Bits & Bytes in Action

## Our Boot Sector Memory Layout

```
Memory Address    Hex Values    ASCII    Purpose
-------------------------------------------------
0x7C00:          31 C0         1.       xor ax, ax
0x7C02:          8E D8         ..       mov ds, ax
0x7C04:          8E C0         ..       mov es, ax
0x7C06:          8E D0         ..       mov ss, ax
0x7C08:          BC 00 7C      ..|      mov sp, 0x7C00
...
0x7DFE:          55 AA         U.       Boot signature
```

## Instruction-by-Instruction Breakdown

### Instruction 1: `xor ax, ax`
```
Machine Code: 31 C0
Binary:       0011 0001 1100 0000

Decoding:
31 = XOR instruction for 16-bit register with register
C0 = ModR/M byte specifying AX with AX

Before execution:
AX = ???? (unknown value)

Operation (bit by bit):
If AX = 0x1234:
0001 0010 0011 0100  (AX)
XOR
0001 0010 0011 0100  (AX)
=
0000 0000 0000 0000  (Result)

After execution:
AX = 0x0000
AH = 0x00, AL = 0x00
```

### Instruction 2: `mov ds, ax`
```
Machine Code: 8E D8
Binary:       1000 1110 1101 1000

Decoding:
8E = MOV to segment register
D8 = ModR/M byte (DS, AX)

Register Transfer:
AX (16-bit) ──────> DS (16-bit)
   0x0000           0x0000

Effect:
Data Segment now points to physical address 0x0000
(In real mode: physical = DS × 16 + offset)
```

### Instruction 3: `mov sp, 0x7C00`
```
Machine Code: BC 00 7C
Binary:       1011 1100 0000 0000 0111 1100

Decoding:
BC = MOV immediate to SP
00 7C = 0x7C00 (little-endian!)

Memory storage (little-endian):
Address  Value
0x7C09:  00    (low byte of 0x7C00)
0x7C0A:  7C    (high byte of 0x7C00)

Stack Pointer Setup:
SP = 0x7C00 (points to our boot sector start)
Stack will grow downward from here
```

## How BIOS Interrupt Works: `int 0x10`

### Setting up for video output:
```asm
mov ah, 0x0E    ; Teletype output function
mov al, 'B'     ; Character to print
int 0x10        ; Call BIOS
```

### Register State Before Interrupt:
```
AX = 0x0E42
     ├─ AH = 0x0E (function number)
     └─ AL = 0x42 (ASCII 'B')

Binary view of AX:
0000 1110 0100 0010
│       │ │       │
│   0E  │ │  42   │
└───AH──┘ └───AL──┘
```

### What Happens During `int 0x10`:
1. **CPU saves state**:
   ```
   PUSH FLAGS
   PUSH CS
   PUSH IP
   ```

2. **CPU jumps to interrupt handler**:
   ```
   Interrupt Vector Table (IVT) at 0x0000:
   Entry 0x10 is at address 0x10 × 4 = 0x40
   
   Memory at 0x0040: [IP_low][IP_high][CS_low][CS_high]
   CPU loads: CS:IP = interrupt handler address
   ```

3. **BIOS code reads AH**:
   ```
   CMP AH, 0x0E
   JE teletype_output
   ```

4. **Outputs character in AL**:
   ```
   - Writes to video memory at 0xB8000
   - Updates cursor position
   ```

## String Printing Deep Dive

```asm
mov si, boot_msg    ; SI = address of string
call print_string
```

### Memory Layout of String:
```
Address    Hex    Binary        ASCII   Meaning
------------------------------------------------------
0x7C50:    42     0100 0010     'B'     First character
0x7C51:    6F     0110 1111     'o'     
0x7C52:    6F     0110 1111     'o'
0x7C53:    74     0111 0100     't'
0x7C54:    69     0110 1001     'i'
0x7C55:    6E     0110 1110     'n'
0x7C56:    67     0110 0111     'g'
0x7C57:    20     0010 0000     ' '     Space
0x7C58:    4F     0100 1111     'O'
0x7C59:    53     0101 0011     'S'
0x7C5A:    2E     0010 1110     '.'
0x7C5B:    2E     0010 1110     '.'
0x7C5C:    2E     0010 1110     '.'
0x7C5D:    0D     0000 1101     CR      Carriage Return
0x7C5E:    0A     0000 1010     LF      Line Feed
0x7C5F:    00     0000 0000     NULL    String terminator
```

### LODSB Instruction Analysis:
```asm
lodsb    ; Load String Byte
```

What it does (pseudo-code):
```
AL = Memory[DS:SI]  ; Load byte at DS:SI into AL
SI = SI + 1         ; Increment SI (or decrement if DF=1)
```

Execution trace:
```
Iteration 1: SI=0x7C50, AL='B' (0x42), SI becomes 0x7C51
Iteration 2: SI=0x7C51, AL='o' (0x6F), SI becomes 0x7C52
...
Iteration N: SI=0x7C5F, AL=0x00 (NULL), SI becomes 0x7C60
```

## The Stack in Action

### Initial Stack Setup:
```
mov ss, ax      ; SS = 0x0000
mov sp, 0x7C00  ; SP = 0x7C00
```

### Stack Memory Visualization:
```
Address    Content          Stack Growth
0x7C00:    [boot sector]    <- SP starts here
0x7BFE:    [empty]          <- After first PUSH (16-bit)
0x7BFC:    [empty]          <- After second PUSH
0x7BFA:    [empty]          ↑ Stack grows upward
...                         | (toward lower addresses)
```

### PUSHA Instruction (Save All Registers):
```asm
pusha    ; Push all general registers
```

Order of pushing (16-bit mode):
```
PUSH AX  ; SP = SP - 2, [SS:SP] = AX
PUSH CX  ; SP = SP - 2, [SS:SP] = CX
PUSH DX  ; SP = SP - 2, [SS:SP] = DX
PUSH BX  ; SP = SP - 2, [SS:SP] = BX
PUSH SP  ; SP = SP - 2, [SS:SP] = original SP
PUSH BP  ; SP = SP - 2, [SS:SP] = BP
PUSH SI  ; SP = SP - 2, [SS:SP] = SI
PUSH DI  ; SP = SP - 2, [SS:SP] = DI

Total: 16 bytes pushed (8 registers × 2 bytes)
```

## Boot Signature Analysis

### The Final Two Bytes:
```asm
times 510-($-$$) db 0  ; Padding
dw 0xAA55              ; Boot signature
```

### How `times` Works:
```
$ = current position in file
$$ = start of current section (0 in our case)
$-$$ = bytes used so far

510-($-$$) = bytes of padding needed

If we've used 100 bytes:
510 - 100 = 410 bytes of zeros
```

### Memory at End of Boot Sector:
```
Offset   Hex    Binary         Purpose
---------------------------------------
0x1FC:   00     0000 0000     Padding
0x1FD:   00     0000 0000     Padding
0x1FE:   55     0101 0101     Boot signature (low byte)
0x1FF:   AA     1010 1010     Boot signature (high byte)

Total: 512 bytes (0x200 in hex)
```

### Why Little-Endian Matters:
```
We write: dw 0xAA55
Stored as: 55 AA (reversed!)

Word value: 0xAA55
High byte:  0xAA
Low byte:   0x55

In memory (little-endian):
[Low byte][High byte]
[  0x55  ][  0xAA   ]
```

## CPU Mode and Register Sizes

### Real Mode (16-bit) - What We're Using:
```
Available Registers: AX, BX, CX, DX, SI, DI, BP, SP
Max addressable memory: 1MB (20-bit addressing)
Segment:Offset addressing: Physical = Segment×16 + Offset

Example:
CS = 0x0000, IP = 0x7C00
Physical address = 0x0000×16 + 0x7C00 = 0x07C00
```

### Protected Mode (32-bit) - Where We're Going:
```
Available Registers: EAX, EBX, ECX, EDX, ESI, EDI, EBP, ESP
Max addressable memory: 4GB (32-bit addressing)
Flat memory model: Direct 32-bit addresses
```

### Long Mode (64-bit) - Final Destination:
```
Available Registers: RAX, RBX, RCX, RDX, RSI, RDI, RBP, RSP, R8-R15
Max addressable memory: 16EB (48-bit addressing in practice)
64-bit flat memory model
```

## Debugging Tips

### Reading Register Values:
```
In QEMU monitor (Ctrl+Alt+2):
(qemu) info registers

Output:
EAX=00000000 EBX=00000000 ECX=00000000 EDX=00000080
ESI=00000000 EDI=00000000 EBP=00000000 ESP=00007c00
...
```

### Examining Memory:
```
(qemu) x/16bx 0x7c00
Shows 16 bytes in hex starting at 0x7C00

(qemu) x/10i 0x7c00
Shows 10 instructions starting at 0x7C00
```

### Common Issues and Solutions:

1. **Wrong segment setup**:
   ```
   Problem: Data not found
   Check: DS register value
   Fix: Ensure DS points to correct segment
   ```

2. **Stack overflow**:
   ```
   Problem: Corrupted code
   Check: SP value after operations
   Fix: Ensure stack doesn't grow into code
   ```

3. **Wrong addressing mode**:
   ```
   Problem: Loading wrong values
   Check: Using [] for memory access
   Fix: mov ax, [variable] not mov ax, variable
   ```

This detailed analysis shows exactly how bits and bytes work together with registers to make our boot sector function!