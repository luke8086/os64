OUTPUT_FORMAT(elf64-x86-64)
ENTRY(entry)

APP_START = 0x10000000;

SECTIONS {

    . = APP_START;

    .text : {
        *(.text)
    }

    .data : {
        *(.data)
        *(.rodata)
        *(.eh_frame)
    }

    .bss : ALIGN(0x1000) {
        *(.bss)
    }

}
