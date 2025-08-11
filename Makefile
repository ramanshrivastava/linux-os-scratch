# OS From Scratch - Makefile
# Build system for our operating system

# Tools
AS = nasm
QEMU = qemu-system-x86_64

# Directories
BOOT_DIR = boot
BUILD_DIR = build
KERNEL_DIR = kernel
DRIVERS_DIR = drivers

# Source files
BOOT_SRC = $(BOOT_DIR)/boot.asm

# Output files
BOOT_BIN = $(BUILD_DIR)/boot.bin
OS_IMG = $(BUILD_DIR)/os.img

# Default target
all: $(OS_IMG)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Assemble boot sector
$(BOOT_BIN): $(BOOT_SRC) | $(BUILD_DIR)
	@echo "Assembling boot sector..."
	$(AS) -f bin $< -o $@
	@echo "Boot sector size: $$(stat -f%z $@ 2>/dev/null || stat -c%s $@ 2>/dev/null) bytes"
	@echo "Checking boot signature..."
	@hexdump -C $@ | tail -n 2 | grep -q "55 aa" && echo "✓ Boot signature found" || echo "✗ Boot signature missing!"

# Create disk image (will expand as we add more components)
$(OS_IMG): $(BOOT_BIN)
	@echo "Creating OS disk image..."
	@# Create 10MB disk image filled with zeros
	dd if=/dev/zero of=$@ bs=1M count=10 status=none
	@# Write boot sector to first 512 bytes
	dd if=$(BOOT_BIN) of=$@ conv=notrunc status=none
	@echo "✓ OS image created: $@"

# Run in QEMU with graphical display
run: $(OS_IMG)
	@echo "Starting OS in QEMU..."
	@echo "Press Ctrl+A then X to exit"
	$(QEMU) -drive format=raw,file=$(OS_IMG) -m 32M

# Run in QEMU with curses display (terminal-based)
run-curses: $(OS_IMG)
	@echo "Starting OS in QEMU (curses mode)..."
	@echo "Press ESC+2 then type 'quit' to exit"
	$(QEMU) -drive format=raw,file=$(OS_IMG) -m 32M -display curses

# Run in QEMU with no display (serial console)
run-serial: $(OS_IMG)
	@echo "Starting OS in QEMU (serial console)..."
	@echo "Press Ctrl+A then X to exit"
	$(QEMU) -drive format=raw,file=$(OS_IMG) -m 32M -nographic -serial mon:stdio

# Debug with QEMU monitor
debug: $(OS_IMG)
	@echo "Starting OS in debug mode..."
	@echo "QEMU monitor on stdio, GDB server on :1234"
	$(QEMU) -drive format=raw,file=$(OS_IMG) -m 32M -monitor stdio -s -S

# Quick test - just check if it assembles
test: $(BOOT_BIN)
	@echo "✓ Boot sector assembled successfully"
	@echo "Run 'make run' to test in QEMU"

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BUILD_DIR)
	rm -f $(BOOT_DIR)/*.bin
	@echo "✓ Clean complete"

# Show project info
info:
	@echo "=== OS From Scratch Build Info ==="
	@echo "NASM version: $$(nasm -v | head -n 1)"
	@echo "QEMU version: $$(qemu-system-x86_64 --version | head -n 1)"
	@echo ""
	@echo "Project structure:"
	@find . -type f -name "*.asm" -o -name "*.c" -o -name "*.h" | head -20
	@echo ""
	@echo "Available targets:"
	@echo "  make all       - Build OS image"
	@echo "  make run       - Run in QEMU"
	@echo "  make test      - Quick assembly test"
	@echo "  make clean     - Remove build artifacts"
	@echo "  make info      - Show this info"

.PHONY: all run run-curses run-serial debug test clean info