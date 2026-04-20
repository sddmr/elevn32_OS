


elevn32 OS

Current Version: v0.4.0

elevn32 OS is a 64-bit, bare-metal operating system written from scratch using C++ and x86_64 Assembly. The main goal of this project is to deeply understand low-level system design, hardware interaction, and operating system fundamentals by building everything without relying on existing operating systems or high-level frameworks.

⸻

Overview

This project focuses on building a minimal yet functional operating system starting from the boot process up to a basic graphical user interface. Every component is implemented manually, including memory management, interrupt handling, hardware drivers, and a custom window manager.

The system runs in a virtualized environment using QEMU and is compiled with a custom cross-compiler toolchain.

⸻

Features

Kernel & Core Systems

* Custom 64-bit kernel written in C++ and Assembly (x86_64)
* Boot process handled via Limine bootloader
* Physical Memory Management (PMM) with page-frame allocation
* Interrupt handling using GDT, IDT, and ISR structures
* Programmable Interrupt Controller (PIC) configuration
* Low-level hardware interaction via port I/O

Hardware Drivers

* PS/2 keyboard driver (scancode handling, input processing)
* PS/2 mouse driver (basic cursor support)

Graphics & GUI

* VESA-compatible framebuffer graphics engine
* Double buffering to eliminate screen tearing
* Custom event-driven window manager
* Draggable windows, menubar, and dock system
* Basic desktop environment

System Components

* Minimal terminal (shell-like interface)
* Settings module (runtime changes such as background color)
* Custom asset pipeline that parses PNG files at build time and embeds them into the kernel (temporary solution before filesystem implementation)

⸻

Build & Run

Requirements

* x86_64-elf-gcc cross compiler
* make / gmake
* qemu-system-x86_64
* Python (for asset pipeline)

Build

make

Run

make run

⸻

Project Goals

* Understand low-level OS architecture and design
* Gain hands-on experience with memory management and interrupts
* Explore hardware-software interaction at the bare-metal level
* Build a fully custom graphical environment from scratch

⸻

Future Work

* Filesystem implementation
* Process and multitasking system
* Virtual memory improvements
* User-space applications
* USB and storage driver support
* Networking stack

⸻

Notes

This project is purely educational and experimental. It is not intended for production use, but rather as a deep dive into operating system internals and low-level programming.

⸻

Contributing

Contributions, suggestions, and discussions are welcome.

⸻

