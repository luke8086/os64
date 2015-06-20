OUTPUT_FORMAT(elf64-x86-64)
ENTRY(entry)

KERNEL_START = 0x00100000;

SECTIONS {

    . = KERNEL_START;

    kernel_start = .;

    .text : {
        kernel_text_start = .;
        *(.text)
    }
    kernel_text_end = .;

    .data : {
        kernel_data_start = .;
        *(.data)
        *(.rodata)
        *(.eh_frame)
    }
    kernel_data_end = .;

    .bss : ALIGN(0x1000) {
        kernel_bss_start = .;
        *(.bss)
    }
    kernel_bss_end = .;

    kernel_end = .;

    /DISCARD/ : {
        *(.comment)
    }

}
