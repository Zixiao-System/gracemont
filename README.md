# Gracemont Industrial Control Framework

An open-source industrial control system framework supporting Soft PLC, DCS, and SCADA/HMI applications with real-time operating system (RTOS) support for ARM64 and x86_64 architectures.

## Features

- **Real-Time Operating System (RTOS)**
  - ARM64 and x86_64 architecture support
  - Deterministic task scheduling
  - Memory management with MPU support
  - Inter-task synchronization primitives

- **Industrial Protocols**
  - Modbus TCP/RTU
  - OPC UA
  - PROFINET

- **Soft PLC Runtime**
  - IEC 61131-3 compatible
  - Ladder Diagram (LD) support
  - Function Block Diagram (FBD) support
  - Structured Text (ST) support

- **Distributed Control System (DCS)**
  - Multi-node coordination
  - Redundancy support
  - Alarm management

- **HMI/SCADA**
  - Web-based visualization
  - Real-time data trending
  - Historical data logging

## Project Structure

```
Gracemont-Industrial-Control-framework/
├── ARM64-RTOS/          # ARM64 RTOS implementation
│   ├── arch/            # Architecture-specific code
│   ├── kernel/          # Kernel core (scheduler, memory, sync)
│   ├── drivers/         # Device drivers (UART, Timer, Ethernet)
│   ├── net/             # Network stack
│   ├── protocols/       # Industrial protocols (Modbus, OPC UA, PROFINET)
│   ├── include/         # Header files
│   ├── config/          # Configuration files
│   ├── tests/           # Unit tests
│   └── build/           # Build output
├── X86_64-RTOS/         # x86_64 RTOS implementation
├── SoftPLC/             # Soft PLC runtime
│   ├── runtime/         # PLC runtime engine
│   ├── compiler/        # IEC 61131-3 compiler
│   ├── stdlib/          # Standard function blocks
│   └── programs/        # Example PLC programs
├── DCS/                 # Distributed Control System
│   ├── controller/      # DCS controller logic
│   ├── io/              # I/O management
│   ├── redundancy/      # Redundancy management
│   └── alarm/           # Alarm subsystem
├── HMI/                 # Human-Machine Interface
│   ├── server/          # HMI server
│   ├── web/             # Web-based UI
│   ├── widgets/         # UI widgets
│   └── historian/       # Data historian
├── devtools/            # Development tools
│   ├── simulator/       # Hardware simulator
│   ├── debugger/        # Debug utilities
│   └── flasher/         # Firmware flashing tools
├── docs/                # Documentation
│   ├── architecture/    # Architecture documents
│   ├── api/             # API reference
│   └── tutorials/       # Tutorials and guides
└── examples/            # Example projects
```

## Getting Started

### Prerequisites

- GCC ARM toolchain (for ARM64 builds)
- GCC (for x86_64 builds)
- Make
- QEMU (for simulation)

### Building

```bash
# Build ARM64 RTOS
cd ARM64-RTOS
make

# Run tests
make test

# Run in QEMU
make qemu
```

```bash
# Build X86_64 RTOS
cd X86_64-RTOS
make menuconfig  # Configure build options (optional)
make

# Run in QEMU
make qemu
```

### Quick Start

1. Clone the repository
   ```bash
   git clone https://github.com/Zixiao-System/gracemont.git
   cd gracemont
   ```

2. Build the RTOS
   ```bash
   cd ARM64-RTOS
   make
   ```

3. Run the example
   ```bash
   make qemu
   ```

## Documentation

See the [docs](docs/) directory for detailed documentation:

- [Architecture Overview](docs/architecture/)
- [API Reference](docs/api/)
- [Tutorials](docs/tutorials/)

## Contributing

We welcome contributions! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## License

This project is licensed under the GNU General Public License v3.0 - see the [LICENSE](LICENSE) file for details.

## Roadmap

- [ ] Complete ARM64 RTOS kernel
- [x] Implement x86_64 RTOS port
- [ ] Soft PLC runtime with IEC 61131-3 support
- [ ] DCS controller framework
- [ ] Web-based HMI
- [ ] OPC UA server/client
- [ ] Safety-critical certification (IEC 61508)

## Contact

For questions and support, please open an issue on GitHub.
