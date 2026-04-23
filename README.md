# elevn32 OS

**Current Version:** `v1.0.0`

elevn32 OS is a 64-bit bare-metal operating system written from scratch in C++ and x86_64 Assembly. The project is built as a hands-on exploration of low-level system architecture, hardware interaction, and core operating system design without relying on existing OS frameworks.

## Default Login

- **Username:** `Root`
- **Password:** `123`

## Overview

The goal of elevn32 OS is to build a minimal but functional operating system from the ground up, starting at the boot process and extending into a basic graphical desktop environment. Core subsystems such as memory management, interrupt handling, hardware drivers, and window management are implemented manually to better understand how modern systems work internally.

The system runs in a virtualized environment with QEMU and is built using a custom cross-compilation toolchain.

## Features

### Kernel and Core Systems

- Custom 64-bit kernel written in C++ and Assembly
- Boot process powered by the Limine bootloader
- Physical Memory Management (PMM) with page-frame allocation
- Interrupt handling through GDT, IDT, and ISR structures
- PIC configuration and low-level port I/O support

### Hardware Drivers

- PS/2 keyboard driver with scancode handling
- PS/2 mouse driver with basic cursor support

### Graphics and GUI

- VESA-compatible framebuffer renderer
- Double buffering to reduce screen tearing
- Custom event-driven window manager
- Draggable windows, menu bar, and dock interface
- Basic desktop environment

### System Components

- Minimal terminal interface
- Settings module for runtime adjustments such as background color
- Temporary asset pipeline that parses PNG files at build time and embeds them into the kernel

## Build and Run

### Requirements

- `x86_64-elf-gcc` cross compiler
- `make` or `gmake`
- `qemu-system-x86_64`
- `python`

### Build

```bash
make
```

### Run

```bash
make run
```

## Project Goals

- Understand low-level operating system architecture and design
- Gain practical experience with memory management and interrupts
- Explore direct hardware-software interaction on bare metal
- Build a fully custom graphical environment from scratch

## Future Work

- Filesystem implementation
- Process and multitasking improvements
- Further virtual memory development
- User-space applications
- USB and storage driver support
- Networking stack

## Notes

This project is educational and experimental. It is not intended for production use, but as a practical deep dive into operating system internals and low-level programming.

## Contributing

Contributions, suggestions, and discussions are welcome.
