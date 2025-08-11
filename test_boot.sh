#!/bin/bash
# Test script for boot sector

echo "Testing boot sector..."
echo "Press Ctrl+A then X to exit QEMU"
echo ""

# Run QEMU with our boot sector
# Using SDL display which should work on macOS
qemu-system-x86_64 \
    -drive format=raw,file=boot/boot.bin \
    -m 32M \
    -display sdl \
    2>/dev/null || \
qemu-system-x86_64 \
    -drive format=raw,file=boot/boot.bin \
    -m 32M \
    -display cocoa \
    2>/dev/null || \
echo "Note: QEMU window should open separately. If not visible, check your display settings."