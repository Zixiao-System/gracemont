#
# Gracemont Industrial Control Framework - ARM64 RTOS
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
# ARM64 RTOS Makefile
#

# Toolchain
CROSS_COMPILE ?= aarch64-elf-
CC      = $(CROSS_COMPILE)gcc
AS      = $(CROSS_COMPILE)as
LD      = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
SIZE    = $(CROSS_COMPILE)size

# Kconfig tools (uses kconfiglib - install with: pip install kconfiglib)
MENUCONFIG  = menuconfig
GUICONFIG   = guiconfig
GENCONFIG   = scripts/genconfig.py

# Project name
PROJECT = gracemont-rtos

# Directories
SRC_DIR     = .
BUILD_DIR   = build
KERNEL_DIR  = kernel
ARCH_DIR    = arch/arm64
NET_DIR     = net
PROTO_DIR   = protocols
DRIVER_DIR  = drivers
INC_DIR     = include
CONFIG_DIR  = config
CONFIGS_DIR = configs

# Configuration files
DOTCONFIG   = .config
AUTOCONF_H  = $(INC_DIR)/autoconf.h

# Source files
ASM_SOURCES = \
    $(ARCH_DIR)/startup.S

C_SOURCES = \
    $(KERNEL_DIR)/main.c \
    $(KERNEL_DIR)/scheduler.c \
    $(KERNEL_DIR)/sync.c \
    $(KERNEL_DIR)/memory.c \
    $(KERNEL_DIR)/interrupt.c \
    $(ARCH_DIR)/mmu.c \
    $(DRIVER_DIR)/uart/uart.c \
    $(DRIVER_DIR)/eth/eth.c \
    $(NET_DIR)/buffer/zbuf.c \
    $(NET_DIR)/stack/net_core.c \
    $(NET_DIR)/stack/tcp.c \
    $(PROTO_DIR)/modbus/modbus.c \
    $(PROTO_DIR)/opcua/opcua.c \
    $(PROTO_DIR)/profinet/profinet.c

# Test source files
TEST_DIR = tests
TEST_SOURCES = \
    $(TEST_DIR)/test_framework.c \
    $(TEST_DIR)/test_main.c \
    $(TEST_DIR)/test_memory.c \
    $(TEST_DIR)/test_zbuf.c \
    $(TEST_DIR)/test_sync.c \
    $(TEST_DIR)/test_modbus.c

# Include paths
INCLUDES = \
    -I$(INC_DIR) \
    -I$(CONFIG_DIR) \
    -I$(TEST_DIR)

# Compiler flags
CFLAGS = -Wall -Wextra -Werror \
         -ffreestanding -nostdlib -nostartfiles \
         -mcpu=cortex-a53 -march=armv8-a \
         -mgeneral-regs-only \
         -fno-builtin -fno-common \
         -ffunction-sections -fdata-sections \
         -O2 -g \
         $(INCLUDES)

# Assembler flags
ASFLAGS = -mcpu=cortex-a53 -march=armv8-a -g

# Linker flags
LDFLAGS = -T $(CONFIG_DIR)/linker.ld \
          --gc-sections \
          -Map=$(BUILD_DIR)/$(PROJECT).map

# Object files
ASM_OBJECTS = $(addprefix $(BUILD_DIR)/, $(ASM_SOURCES:.S=.o))
C_OBJECTS   = $(addprefix $(BUILD_DIR)/, $(C_SOURCES:.c=.o))
TEST_OBJECTS = $(addprefix $(BUILD_DIR)/, $(TEST_SOURCES:.c=.o))
OBJECTS     = $(ASM_OBJECTS) $(C_OBJECTS)

# Output files
ELF     = $(BUILD_DIR)/$(PROJECT).elf
BIN     = $(BUILD_DIR)/$(PROJECT).bin
HEX     = $(BUILD_DIR)/$(PROJECT).hex
DUMP    = $(BUILD_DIR)/$(PROJECT).dump

# Default target
all: $(AUTOCONF_H) $(BIN) $(HEX) size

# Create build directories
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)
	@mkdir -p $(BUILD_DIR)/$(ARCH_DIR)
	@mkdir -p $(BUILD_DIR)/$(NET_DIR)/buffer
	@mkdir -p $(BUILD_DIR)/$(NET_DIR)/stack
	@mkdir -p $(BUILD_DIR)/$(PROTO_DIR)/modbus
	@mkdir -p $(BUILD_DIR)/$(PROTO_DIR)/opcua
	@mkdir -p $(BUILD_DIR)/$(PROTO_DIR)/profinet
	@mkdir -p $(BUILD_DIR)/$(DRIVER_DIR)/uart
	@mkdir -p $(BUILD_DIR)/$(DRIVER_DIR)/timer
	@mkdir -p $(BUILD_DIR)/$(DRIVER_DIR)/eth
	@mkdir -p $(BUILD_DIR)/$(TEST_DIR)

# Compile assembly files
$(BUILD_DIR)/%.o: %.S | $(BUILD_DIR)
	@echo "  AS    $<"
	@$(CC) $(ASFLAGS) -c $< -o $@

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

# Generate hex
$(HEX): $(ELF)
	@echo "  HEX   $@"
	@$(OBJCOPY) -O ihex $< $@

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

# Clean
clean:
	@echo "  CLEAN"
	@rm -rf $(BUILD_DIR)
	@rm -f $(AUTOCONF_H)

# Clean including config
distclean: clean
	@echo "  DISTCLEAN"
	@rm -f $(DOTCONFIG) $(DOTCONFIG).old

# Run with QEMU (aarch64)
qemu: $(ELF)
	@echo "Starting QEMU..."
	qemu-system-aarch64 \
		-M virt \
		-cpu cortex-a53 \
		-m 512M \
		-nographic \
		-kernel $(ELF) \
		-serial mon:stdio \
		-device virtio-net-device,netdev=net0 \
		-netdev user,id=net0,hostfwd=tcp::5502-:502,hostfwd=tcp::5840-:4840

# Debug with QEMU and GDB
debug: $(ELF)
	@echo "Starting QEMU in debug mode..."
	qemu-system-aarch64 \
		-M virt \
		-cpu cortex-a53 \
		-m 512M \
		-nographic \
		-kernel $(ELF) \
		-serial mon:stdio \
		-S -s &
	@echo "Connect GDB with: aarch64-none-elf-gdb -ex 'target remote localhost:1234' $(ELF)"

# Generate compile_commands.json for IDE support
compile_commands: clean
	@echo "Generating compile_commands.json..."
	@bear -- $(MAKE) all

# Dependencies
-include $(OBJECTS:.o=.d)

.PHONY: all clean distclean dump size qemu debug compile_commands test \
        menuconfig guiconfig defconfig savedefconfig oldconfig silentoldconfig help

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
	qemu-system-aarch64 \
		-M virt \
		-cpu cortex-a53 \
		-m 512M \
		-nographic \
		-kernel $(BUILD_DIR)/$(PROJECT)-test.elf \
		-serial mon:stdio

# ============================================================================
# Kconfig Configuration Targets
# ============================================================================

# Interactive menu-based configuration (ncurses)
menuconfig:
	@echo "Starting menuconfig..."
	@$(MENUCONFIG) Kconfig

# Interactive GUI configuration (Qt/Tk)
guiconfig:
	@echo "Starting guiconfig..."
	@$(GUICONFIG) Kconfig

# Load default configuration
defconfig:
	@echo "Loading default configuration..."
	@cp $(CONFIGS_DIR)/defconfig $(DOTCONFIG)
	@$(MAKE) silentoldconfig

# Save current configuration as default
savedefconfig:
	@echo "Saving configuration to $(CONFIGS_DIR)/defconfig..."
	@cp $(DOTCONFIG) $(CONFIGS_DIR)/defconfig

# Update configuration (interactive)
oldconfig:
	@echo "Running oldconfig..."
	@oldconfig Kconfig

# Update configuration (non-interactive, use defaults)
silentoldconfig: $(DOTCONFIG)
	@echo "Generating $(AUTOCONF_H)..."
	@python3 $(GENCONFIG) $(DOTCONFIG) $(AUTOCONF_H)

# Generate autoconf.h from .config
$(AUTOCONF_H): $(DOTCONFIG)
	@echo "Generating $(AUTOCONF_H)..."
	@python3 $(GENCONFIG) $(DOTCONFIG) $(AUTOCONF_H)

# Create .config if it doesn't exist
$(DOTCONFIG):
	@if [ ! -f $(DOTCONFIG) ]; then \
		echo "No .config found, loading defconfig..."; \
		cp $(CONFIGS_DIR)/defconfig $(DOTCONFIG); \
	fi

# Help target
help:
	@echo ""
	@echo "Gracemont ARM64 RTOS Build System"
	@echo "=================================="
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
	@echo "  oldconfig       - Update config (interactive)"
	@echo "  silentoldconfig - Update config (non-interactive)"
	@echo ""
	@echo "QEMU targets:"
	@echo "  qemu            - Run RTOS in QEMU"
	@echo "  qemu-test       - Run tests in QEMU"
	@echo "  debug           - Start QEMU with GDB server"
	@echo ""
	@echo "Prerequisites:"
	@echo "  pip install kconfiglib"
	@echo ""
