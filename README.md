# elevn32 OS

**Current Version:** `v1.1.1`

elevn32 OS is a 64-bit bare-metal operating system written from scratch in C++ and x86_64 Assembly. It is built as a practical exploration of low-level architecture, memory management, hardware interaction, and desktop environment design without relying on an existing operating system stack.

## Highlights

- Custom x86_64 kernel with manual low-level subsystem development
- Limine-based boot flow and ISO image generation
- Physical and virtual memory groundwork for more advanced kernel features
- Custom framebuffer-based desktop environment and window manager
- Built-in desktop apps including Terminal, Calculator, Paint, Clock, Notepad, Snake, Minesweeper, System Info, Files, and Settings
- Interactive shell support both in the boot console and inside the desktop terminal window
- Theme-aware desktop styling with configurable colors and dark-mode support

## Default Login

- **Username:** `root`
- **Password:** `123`

## Overview

The project aims to build a compact but functional operating system from the boot stage up to a graphical desktop experience. Core pieces such as memory handling, interrupt management, PS/2 input drivers, rendering, shell behavior, and window management are implemented manually to better understand how a real operating system is structured internally.

The current system is designed to run inside QEMU and is built with a dedicated cross-compilation toolchain.

## Features

### Kernel and Core Systems

- Custom 64-bit kernel written in C++ and x86_64 Assembly
- Limine bootloader integration
- Physical Memory Management (PMM)
- Early virtual memory support
- Interrupt handling with GDT, IDT, and ISR infrastructure
- PIC setup and direct port I/O access

### Desktop Environment

- Framebuffer-based graphical desktop
- Custom event-driven window manager
- Draggable window system with active and inactive title states
- Boot screen animation and themed desktop visuals
- Dark-mode aware interface behavior

### Built-In Applications

- Terminal
- Calculator
- Paint
- Clock
- Settings
- Notepad
- System Info
- Snake
- Minesweeper
- Files

### Input and Interaction

- PS/2 keyboard driver
- PS/2 mouse driver
- Desktop-integrated shell command handling

## Build and Run

### Requirements

- `x86_64-elf-gcc` cross compiler
- `gmake`
- `qemu-system-x86_64`
- `xorriso`
- `curl`
- `git`

### Build

```bash
cd op_sys
gmake
```

### Run

```bash
cd op_sys
gmake run
```

### Output

The build produces:

- `op_sys/elevn32.iso`

## Project Goals

- Understand low-level operating system architecture in practice
- Gain hands-on experience with memory, interrupts, and hardware interfaces
- Explore how a graphical desktop can be built directly on bare metal
- Grow the project toward a more complete multitasking operating system

## Future Work

- Filesystem implementation
- More mature process and multitasking systems
- Expanded virtual memory support
- User-space applications
- USB and storage drivers
- Networking stack

## Notes

This project is educational and experimental. It is not intended for production use; it is a learning-focused operating system project built to explore system internals from the ground up.

## Contributing

Contributions, suggestions, and discussions are welcome.
