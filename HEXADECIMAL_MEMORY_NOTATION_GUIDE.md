# Understanding Hexadecimal Notation and Memory Addressing

## What is 0x0E41?

Let's break down `0x0E41` piece by piece:

```
0x0E41
│││└┴┴─ The actual hex digits (0, E, 4, 1)
││└──── Means "hexadecimal" (base 16)
│└───── Just a zero, part of the prefix
└────── The complete prefix "0x"
```

### The "0x" Prefix

**0x** is a prefix that tells us "the following number is in hexadecimal (base 16)"

Different programming conventions:
```
0x0E41  - C, C++, Python, Assembly (most common)
$0E41   - Some assemblers (older style)
0E41h   - Intel assembly syntax
&H0E41  - BASIC
#0E41   - Some calculators
```

Without the prefix, it could be ambiguous:
```
1234  - Is this decimal? Hex? Octal?
0x1234 - Definitely hexadecimal
```

## Number Base Systems Explained

### Decimal (Base 10) - What Humans Use
Uses digits: 0, 1, 2, 3, 4, 5, 6, 7, 8, 9

```
1234 (decimal) means:
1×1000 + 2×100 + 3×10 + 4×1
1×10³  + 2×10² + 3×10¹ + 4×10⁰ = 1234
```

### Binary (Base 2) - What Computers Use
Uses digits: 0, 1

```
1011 (binary) means:
1×8 + 0×4 + 1×2 + 1×1
1×2³ + 0×2² + 1×2¹ + 1×2⁰ = 11 (decimal)

Written as: 0b1011 or 1011b
```

### Hexadecimal (Base 16) - What Programmers Use
Uses digits: 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, A, B, C, D, E, F

```
Hex digit values:
0=0   4=4   8=8    C=12
1=1   5=5   9=9    D=13
2=2   6=6   A=10   E=14
3=3   7=7   B=11   F=15
```

### Why Hexadecimal?

**Each hex digit represents exactly 4 bits:**

```
Hex  Decimal  Binary
0    0        0000
1    1        0001
2    2        0010
3    3        0011
4    4        0100
5    5        0101
6    6        0110
7    7        0111
8    8        1000
9    9        1001
A    10       1010
B    11       1011
C    12       1100
D    13       1101
E    14       1110
F    15       1111
```

**Example: 0x0E41**
```
0x0E41 in different representations:

Hexadecimal: 0x0E41
             0    E    4    1

Binary:      0000 1110 0100 0001

Decimal:     0×4096 + 14×256 + 4×16 + 1×1
             = 0 + 3584 + 64 + 1
             = 3649
```

## Memory Address Notation

### Memory addresses are usually written in hex:

```
0x0000  - Start of memory
0x7C00  - Where BIOS loads boot sector
0xA0000 - Start of VGA graphics memory
0xB8000 - Start of VGA text memory
0xFFFFF - End of first megabyte (real mode limit)
```

### Why These Specific Addresses?

```
0x7C00 breakdown:
7     C     0     0
0111  1100  0000  0000  (binary)

Decimal: 31,744 bytes from start of memory
Why 7C00? Historical: leaves room below for BIOS data
```

## Reading Memory Dumps

When you see a memory dump:

```
Address   Hex Values                ASCII
0x7C00:   31 C0 8E D8 8E C0 8E D0   1.......
0x7C08:   BC 00 7C B8 00 00 B0 03   ..|.....
0x7C10:   CD 10 BE 50 7C E8 08 00   ...P|...
```

Breaking down the format:
```
0x7C00:   31 C0 8E D8 8E C0 8E D0
│         │  │  │  │  │  │  │  │
│         └──┴──┴──┴──┴──┴──┴──┴─ 8 bytes of data
└─────────────────────────────── Memory address
```

Each pair of hex digits = 1 byte:
```
31 = 0011 0001 (binary) = 49 (decimal)
C0 = 1100 0000 (binary) = 192 (decimal)
```

## Common Hex Patterns and Their Meanings

### Zeros and Ones
```
0x00 = 00000000 = 0    = NULL, empty
0xFF = 11111111 = 255  = All bits set, often means -1 in signed
0x01 = 00000001 = 1    = Smallest non-zero value
```

### Powers of 2 (Important in Computing)
```
0x01 = 1     = 2⁰
0x02 = 2     = 2¹
0x04 = 4     = 2²
0x08 = 8     = 2³
0x10 = 16    = 2⁴  (Note: 10 in hex = 16 in decimal!)
0x20 = 32    = 2⁵
0x40 = 64    = 2⁶
0x80 = 128   = 2⁷
0x100 = 256  = 2⁸  (Note: needs 3 hex digits)
```

### Common Memory Sizes
```
0x100    = 256 bytes     = 2⁸
0x400    = 1024 bytes    = 1 KB = 2¹⁰
0x1000   = 4096 bytes    = 4 KB (common page size)
0x10000  = 65536 bytes   = 64 KB (segment size in real mode)
0x100000 = 1048576 bytes = 1 MB
```

### Special Patterns
```
0xAA = 10101010 = Alternating bits pattern
0x55 = 01010101 = Opposite alternating pattern
0xAA55 = Boot signature (little-endian)
0xDEAD = Common debug marker
0xBABE = Another debug marker
0xFEED = Yet another debug marker
0xDEADBEEF = 32-bit debug pattern
```

## Segment:Offset Notation

In real mode (16-bit), we use segment:offset notation:

```
0x07C0:0x0000
│      │
│      └─ Offset (16-bit)
└──────── Segment (16-bit)

Physical address = (Segment × 16) + Offset
                 = (0x07C0 × 0x10) + 0x0000
                 = 0x7C00
```

Why multiply by 16 (0x10)?
- Shifts segment left by 4 bits
- Gives 20-bit addressing from 16-bit values
- Can address 1MB (2²⁰ bytes)

### Same Physical Address, Different Notations:
```
These all point to the same memory location:
0x0000:0x7C00  (segment 0, offset 0x7C00)
0x07C0:0x0000  (segment 0x7C0, offset 0)
0x0700:0x0C00  (segment 0x700, offset 0xC00)

All equal physical address 0x7C00
```

## Converting Between Number Systems

### Hex to Decimal
```
0x2AF =
2×16² + A×16¹ + F×16⁰ =
2×256 + 10×16 + 15×1 =
512 + 160 + 15 = 687
```

### Decimal to Hex
```
687 ÷ 16 = 42 remainder 15 (F)
42 ÷ 16 = 2 remainder 10 (A)
2 ÷ 16 = 0 remainder 2
Read bottom to top: 0x2AF
```

### Binary to Hex (Easy!)
```
Group binary into 4-bit chunks from right:
11001110101 → 110 0111 0101
                6    7    5  → 0x675
```

### Hex to Binary (Also Easy!)
```
Each hex digit becomes 4 bits:
0xA7 → A=1010, 7=0111 → 10100111
```

## Reading Assembly Code Addresses

### Direct Addressing
```asm
mov ax, [0x1234]   ; Load 16-bit value from address 0x1234
```

### Immediate Values
```asm
mov ax, 0x1234     ; Load the VALUE 0x1234 into AX
```

### The Difference is Critical!
```asm
mov ax, 0x7C00     ; AX = 0x7C00 (the number)
mov ax, [0x7C00]   ; AX = whatever 16-bit value is AT address 0x7C00
```

## Port Addresses (I/O Ports)

Hardware devices use port addresses:

```
Common x86 Port Addresses:
0x20-0x21  - Interrupt controller (PIC)
0x40-0x43  - System timer
0x60       - Keyboard data
0x64       - Keyboard status/command
0x3D4-0x3D5 - VGA controller
0x3F8-0x3FF - Serial port COM1
```

Example:
```asm
out 0x60, al   ; Send byte in AL to keyboard controller
in al, 0x60    ; Read byte from keyboard into AL
```

## Practice Examples

### Example 1: What is 0xBEEF?
```
B    E    E    F
11   14   14   15   (decimal values)
1011 1110 1110 1111 (binary)

Decimal: 11×4096 + 14×256 + 14×16 + 15×1
       = 45056 + 3584 + 224 + 15
       = 48,879
```

### Example 2: Memory at 0xB8000
```
0xB8000 = VGA text memory start
        = 753,664 decimal
        = 736 KB into the first megabyte

Each character on screen takes 2 bytes:
Byte 0: ASCII character
Byte 1: Attribute (color)

'A' in white on black at top-left:
Address 0xB8000: 0x41 (ASCII 'A')
Address 0xB8001: 0x07 (white on black)
```

### Example 3: Boot Sector Location
```
0x7C00 = 31,744 decimal
       = About 31 KB into memory

Why not 0x0000?
- 0x0000-0x03FF: Interrupt Vector Table
- 0x0400-0x04FF: BIOS Data Area
- 0x0500-0x7BFF: Free for use
- 0x7C00-0x7DFF: Boot sector (512 bytes)
```

## Quick Reference Card

```
Decimal  Hex   Binary     ASCII
0        0x00  0000 0000  NUL
10       0x0A  0000 1010  LF (newline)
13       0x0D  0000 1101  CR (carriage return)
32       0x20  0010 0000  Space
48       0x30  0011 0000  '0'
65       0x41  0100 0001  'A'
97       0x61  0110 0001  'a'
127      0x7F  0111 1111  DEL
255      0xFF  1111 1111  (all bits set)
```

## Common Mistakes

1. **Forgetting 0x prefix**
   ```
   mov ax, 1000   ; Decimal 1000
   mov ax, 0x1000 ; Hex 1000 = Decimal 4096
   ```

2. **Confusing hex digits**
   ```
   10 in hex = 16 in decimal (not 10!)
   20 in hex = 32 in decimal (not 20!)
   ```

3. **Case sensitivity**
   ```
   0xABCD = 0xabcd = 0xAbCd  (all the same)
   But be consistent for readability
   ```

This notation system makes it easy to see bit patterns and memory alignments, which is why programmers prefer hexadecimal!