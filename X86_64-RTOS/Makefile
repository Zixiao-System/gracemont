#
# Gracemont Industrial Control Framework - X86_64 RTOS
# Copyright (C) 2024 Zixiao System <https://github.com/Zixiao-System>
#
# This file is part of Gracemont Industrial Control Framework.
# Repository: https://github.com/Zixiao-System/gracemont
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# X86_64 RTOS Makefile (LLVM/Clang Toolchain)
#

# ============================================================================
# Toolchain Configuration (LLVM/Clang)
# ============================================================================

# LLVM path (Homebrew on macOS)
LLVM_PATH ?= /opt/homebrew/opt/llvm/bin

# Compilers and tools
CC      = $(LLVM_PATH)/clang
LD      = /opt/homebrew/bin/ld.lld
OBJCOPY = $(LLVM_PATH)/llvm-objcopy
OBJDUMP = $(LLVM_PATH)/llvm-objdump
SIZE    = $(LLVM_PATH)/llvm-size
AR      = $(LLVM_PATH)/llvm-ar


# NASM for x86_64 assembly
NASM    = nasm

# Kconfig tools (uses kconfiglib - install with: pip install kconfiglib)
MENUCONFIG  = menuconfig
GUICONFIG   = guiconfig
GENCONFIG   = scripts/genconfig.py

# Project name
PROJECT = gracemont-rtos-x86_64

# ============================================================================
# Directories
# ============================================================================

SRC_DIR     = .
BUILD_DIR   = build
KERNEL_DIR  = kernel
ARCH_DIR    = arch/x86_64
NET_DIR     = net
PROTO_DIR   = protocols
DRIVER_DIR  = drivers
INC_DIR     = include
CONFIG_DIR  = config
CONFIGS_DIR = configs

# Configuration files
DOTCONFIG   = .config
AUTOCONF_H  = $(INC_DIR)/autoconf.h

# ============================================================================
# Source Files
# ============================================================================

# Assembly sources (NASM format)
NASM_SOURCES = \
    $(ARCH_DIR)/boot/multiboot2.asm \
    $(ARCH_DIR)/boot/long_mode.asm \
    $(ARCH_DIR)/interrupt_vectors.asm \
    $(ARCH_DIR)/context.asm

# C sources
C_SOURCES = \
    $(KERNEL_DIR)/main.c \
    $(KERNEL_DIR)/scheduler.c \
    $(KERNEL_DIR)/sync.c \
    $(KERNEL_DIR)/memory.c \
    $(KERNEL_DIR)/interrupt.c \
    $(ARCH_DIR)/gdt.c \
    $(ARCH_DIR)/idt.c \
    $(ARCH_DIR)/mmu.c \
    $(ARCH_DIR)/apic.c \
    $(ARCH_DIR)/acpi.c \
    $(ARCH_DIR)/cpu.c \
    $(DRIVER_DIR)/serial/uart_16550.c \
    $(DRIVER_DIR)/timer/pit.c \
    $(DRIVER_DIR)/timer/apic_timer.c

# Test source files
TEST_DIR = tests
TEST_SOURCES = \
    $(TEST_DIR)/test_framework.c \
    $(TEST_DIR)/test_main.c \
    $(TEST_DIR)/test_memory.c \
    $(TEST_DIR)/test_zbuf.c \
    $(TEST_DIR)/test_sync.c \
    $(TEST_DIR)/test_modbus.c

# ============================================================================
# Compiler Flags
# ============================================================================

# Include paths
INCLUDES = \
    -I$(INC_DIR) \
    -I$(CONFIG_DIR) \
    -I$(TEST_DIR)

# C compiler flags (for x86_64 freestanding)
CFLAGS = -Wall -Wextra -Werror \
         -Wno-unused-command-line-argument \
         -ffreestanding \
         -target x86_64-unknown-none \
         -mno-red-zone \
         -mno-sse -mno-sse2 -mno-mmx -mno-80387 \
         -mcmodel=kernel \
         -fno-builtin -fno-common -fno-stack-protector \
         -ffunction-sections -fdata-sections \
         -fno-pic -fno-pie \
         -O2 -g \
         $(INCLUDES)

# NASM flags (64-bit ELF output)
NASMFLAGS = -f elf64 -g -F dwarf

# Linker flags
LDFLAGS = -T $(CONFIG_DIR)/linker.ld \
          --gc-sections \
          -z noexecstack \
          -Map=$(BUILD_DIR)/$(PROJECT).map

# ============================================================================
# Object Files
# ============================================================================

NASM_OBJECTS = $(addprefix $(BUILD_DIR)/, $(NASM_SOURCES:.asm=.o))
C_OBJECTS    = $(addprefix $(BUILD_DIR)/, $(C_SOURCES:.c=.o))
TEST_OBJECTS = $(addprefix $(BUILD_DIR)/, $(TEST_SOURCES:.c=.o))
OBJECTS      = $(NASM_OBJECTS) $(C_OBJECTS)

# Output files
ELF     = $(BUILD_DIR)/$(PROJECT).elf
BIN     = $(BUILD_DIR)/$(PROJECT).bin
ISO     = $(BUILD_DIR)/$(PROJECT).iso
DUMP    = $(BUILD_DIR)/$(PROJECT).dump

# ============================================================================
# Build Targets
# ============================================================================

# Default target
all: $(AUTOCONF_H) $(BIN) $(ISO) size

# Create build directories
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)
	@mkdir -p $(BUILD_DIR)/$(ARCH_DIR)/boot
	@mkdir -p $(BUILD_DIR)/$(NET_DIR)/buffer
	@mkdir -p $(BUILD_DIR)/$(NET_DIR)/stack
	@mkdir -p $(BUILD_DIR)/$(PROTO_DIR)/modbus
	@mkdir -p $(BUILD_DIR)/$(PROTO_DIR)/opcua
	@mkdir -p $(BUILD_DIR)/$(PROTO_DIR)/profinet
	@mkdir -p $(BUILD_DIR)/$(DRIVER_DIR)/serial
	@mkdir -p $(BUILD_DIR)/$(DRIVER_DIR)/timer
	@mkdir -p $(BUILD_DIR)/$(DRIVER_DIR)/eth
	@mkdir -p $(BUILD_DIR)/$(TEST_DIR)
	@mkdir -p $(BUILD_DIR)/iso/boot/grub

# Compile NASM assembly files
$(BUILD_DIR)/%.o: %.asm | $(BUILD_DIR)
	@echo "  NASM  $<"
	@$(NASM) $(NASMFLAGS) $< -o $@

# Compile C files
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@echo "  CC    $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# Link
$(ELF): $(OBJECTS)
	@echo "  LD    $@"
	@$(LD) $(LDFLAGS) $(OBJECTS) -o $@

# Generate binary

$(BIN): $(ELF)
	@echo "  BIN   $@"
	@$(OBJCOPY) -O binary $< $@

# Generate bootable ISO (Multiboot2)
$(ISO): $(ELF)
	@echo "  ISO   $@"
	@mkdir -p $(BUILD_DIR)/iso/boot/grub
	@cp $(ELF) $(BUILD_DIR)/iso/boot/kernel.elf
	@echo 'set timeout=0' > $(BUILD_DIR)/iso/boot/grub/grub.cfg
	@echo 'set default=0' >> $(BUILD_DIR)/iso/boot/grub/grub.cfg
	@echo '' >> $(BUILD_DIR)/iso/boot/grub/grub.cfg
	@echo 'menuentry "Gracemont X86_64 RTOS" {' >> $(BUILD_DIR)/iso/boot/grub/grub.cfg
	@echo '    multiboot2 /boot/kernel.elf' >> $(BUILD_DIR)/iso/boot/grub/grub.cfg
	@echo '    boot' >> $(BUILD_DIR)/iso/boot/grub/grub.cfg
	@echo '}' >> $(BUILD_DIR)/iso/boot/grub/grub.cfg
	@x86_64-elf-grub-mkrescue -o $@ $(BUILD_DIR)/iso 2>/dev/null || \
		echo "Warning: Could not create ISO. Install x86_64-elf-grub."

# Generate disassembly
dump: $(ELF)
	@echo "  DUMP  $(DUMP)"
	@$(OBJDUMP) -d -S $< > $(DUMP)

# Print size
size: $(ELF)
	@echo ""
	@echo "Memory Usage:"
	@$(SIZE) $<
	@echo ""

# ============================================================================
# Clean Targets
# ============================================================================

clean:
	@echo "  CLEAN"
	@rm -rf $(BUILD_DIR)
	@rm -f $(AUTOCONF_H)

distclean: clean
	@echo "  DISTCLEAN"
	@rm -f $(DOTCONFIG) $(DOTCONFIG).old

# ============================================================================
# QEMU Targets
# ============================================================================

# Run with QEMU (x86_64) using multiboot
qemu: $(ELF)
	@echo "Starting QEMU (x86_64)..."
	qemu-system-x86_64 \
		-kernel $(ELF) \
		-m 512M \
		-cpu qemu64 \
		-serial mon:stdio \
		-no-reboot \
		-no-shutdown \
		-device virtio-net-pci,netdev=net0 \
		-netdev user,id=net0,hostfwd=tcp::5502-:502,hostfwd=tcp::5840-:4840

# Run with ISO
qemu-iso: $(ISO)
	@echo "Starting QEMU with ISO..."
	qemu-system-x86_64 \
		-cdrom $(ISO) \
		-m 512M \
		-cpu qemu64 \
		-serial mon:stdio \
		-no-reboot \
		-no-shutdown

# Debug with QEMU and GDB/LLDB
debug: $(ELF)
	@echo "Starting QEMU in debug mode..."
	@echo "Connect with: lldb $(ELF) -o 'gdb-remote localhost:1234'"
	@echo "Or: gdb -ex 'target remote localhost:1234' $(ELF)"
	qemu-system-x86_64 \
		-kernel $(ELF) \
		-m 512M \
		-cpu qemu64 \
		-serial mon:stdio \
		-no-reboot \
		-no-shutdown \
		-S -gdb tcp::1234

# ============================================================================
# Test Targets
# ============================================================================

# Build with tests
test: CFLAGS += -DBUILD_TESTS
test: $(BUILD_DIR)/$(PROJECT)-test.elf
	@echo ""
	@echo "Test build complete: $(BUILD_DIR)/$(PROJECT)-test.elf"
	@echo "Run with: make qemu-test"

$(BUILD_DIR)/$(PROJECT)-test.elf: $(OBJECTS) $(TEST_OBJECTS)
	@echo "  LD    $@"
	@$(LD) $(LDFLAGS) $(OBJECTS) $(TEST_OBJECTS) -o $@

# Run tests with QEMU
qemu-test: $(BUILD_DIR)/$(PROJECT)-test.elf
	@echo "Starting QEMU with tests..."
	qemu-system-x86_64 \
		-kernel $(BUILD_DIR)/$(PROJECT)-test.elf \
		-m 512M \
		-cpu qemu64 \
		-serial mon:stdio \
		-no-reboot \
		-no-shutdown

# ============================================================================
# Kconfig Configuration Targets
# ============================================================================

menuconfig:
	@echo "Starting menuconfig..."
	@$(MENUCONFIG) Kconfig

guiconfig:
	@echo "Starting guiconfig..."
	@$(GUICONFIG) Kconfig

defconfig:
	@echo "Loading default configuration..."
	@cp $(CONFIGS_DIR)/defconfig $(DOTCONFIG)
	@$(MAKE) silentoldconfig

savedefconfig:
	@echo "Saving configuration to $(CONFIGS_DIR)/defconfig..."
	@cp $(DOTCONFIG) $(CONFIGS_DIR)/defconfig

oldconfig:
	@echo "Running oldconfig..."
	@oldconfig Kconfig

silentoldconfig: $(DOTCONFIG)
	@echo "Generating $(AUTOCONF_H)..."
	@python3 $(GENCONFIG) $(DOTCONFIG) $(AUTOCONF_H)

$(AUTOCONF_H): $(DOTCONFIG)
	@echo "Generating $(AUTOCONF_H)..."
	@python3 $(GENCONFIG) $(DOTCONFIG) $(AUTOCONF_H)

$(DOTCONFIG):
	@if [ ! -f $(DOTCONFIG) ]; then \
		echo "No .config found, loading defconfig..."; \
		cp $(CONFIGS_DIR)/defconfig $(DOTCONFIG); \
	fi

# ============================================================================
# Dependencies
# ============================================================================

-include $(OBJECTS:.o=.d)

# ============================================================================
# Phony Targets
# ============================================================================

.PHONY: all clean distclean dump size qemu qemu-iso debug test qemu-test \
        menuconfig guiconfig defconfig savedefconfig oldconfig silentoldconfig help

# ============================================================================
# Help
# ============================================================================

help:
	@echo ""
	@echo "Gracemont X86_64 RTOS Build System (LLVM/Clang)"
	@echo "================================================"
	@echo ""
	@echo "Build targets:"
	@echo "  all             - Build the RTOS (default)"
	@echo "  clean           - Remove build artifacts"
	@echo "  test            - Build with unit tests"
	@echo "  dump            - Generate disassembly"
	@echo "  size            - Show memory usage"
	@echo ""
	@echo "Configuration targets:"
	@echo "  menuconfig      - Interactive menu configuration (ncurses)"
	@echo "  guiconfig       - Interactive GUI configuration"
	@echo "  defconfig       - Load default configuration"
	@echo "  savedefconfig   - Save current config as default"
	@echo ""
	@echo "QEMU targets:"
	@echo "  qemu            - Run RTOS in QEMU (multiboot)"
	@echo "  qemu-iso        - Run RTOS from ISO"
	@echo "  qemu-test       - Run tests in QEMU"
	@echo "  debug           - Start QEMU with GDB server"
	@echo ""
	@echo "Prerequisites:"
	@echo "  brew install llvm nasm qemu"
	@echo "  pip install kconfiglib"
	@echo ""
