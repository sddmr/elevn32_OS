/* Linker script for SdmrOS kernel (x86_64) — Limine v8 uyumlu */

OUTPUT_FORMAT(elf64-x86-64)

ENTRY(kernel_start)

PHDRS {
    requests PT_LOAD FLAGS((1 << 2));              /* Read only */
    text     PT_LOAD FLAGS((1 << 0) | (1 << 2));  /* Execute + Read */
    rodata   PT_LOAD FLAGS((1 << 2));               /* Read only */
    data     PT_LOAD FLAGS((1 << 1) | (1 << 2));   /* Write + Read */
}

SECTIONS {
    . = 0xffffffff80000000;

    .limine_requests : {
        KEEP(*(.limine_requests_start))
        KEEP(*(.limine_requests))
        KEEP(*(.limine_requests_end))
    } :requests

    . = ALIGN(4096);

    .text : {
        *(.text .text.*)
    } :text

    . = ALIGN(4096);

    .rodata : {
        *(.rodata .rodata.*)
    } :rodata

    . = ALIGN(4096);

    .data : {
        *(.data .data.*)

        PROVIDE_HIDDEN(__init_array_start = .);
        *(.init_array)
        PROVIDE_HIDDEN(__init_array_end = .);
    } :data

    . = ALIGN(4096);

    .bss : {
        *(.bss .bss.*)
        *(COMMON)
    } :data

    /DISCARD/ : {
        *(.eh_frame*)
        *(.note .note.*)
    }
}
