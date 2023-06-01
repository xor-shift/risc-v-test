OUTPUT_ARCH( "riscv" )
ENTRY( _custom_start )

MEMORY {
    FLASH (rx)  : ORIGIN = 0x00000, LENGTH = 128K
    RAM   (xrw) : ORIGIN = 0x20000, LENGTH = 128K
}

__stack_size = 0x800;
#_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS {
    .init : {
        . = ALIGN(2);

        *(.init)
    }

    .text : {
        . = ALIGN(2);

        *(.text)
        *(.text*)

        KEEP (*(.init))
        KEEP (*(.fini))

        __text_end = .;
    } >FLASH

    .rodata : {
        . = ALIGN(8);
        *(.rodata .rodata.*)
        *(.srodata .srodata.*)
    } >FLASH

    .preinit_array : {
        . = ALIGN(8);
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array*))
        PROVIDE_HIDDEN(__preinit_array_end = .);
    } >FLASH

    .init_array : {
        . = ALIGN(8);
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array*))
        PROVIDE_HIDDEN(__init_array_end = .);
    } >FLASH

    .fini_array : {
        . = ALIGN(8);
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array*))
        PROVIDE_HIDDEN(__fini_array_end = .);
    } >FLASH

    __data_start_flash = LOADADDR(.data);
    .data : {
        . = ALIGN(8);
        __data_start_ram = .;
        *(.data)
        *(.data.*)
        . = ALIGN(8);
        PROVIDE(__global_pointer$ = . + 0x800); # why?
        *(.sdata)
        *(.sdata.*)
        . = ALIGN(8);
        __data_end_ram = .;
        _edata = __data_end_ram;
    } >RAM AT> FLASH

    .bss : {
        . = ALIGN(8);
        __bss_start = .;
        *(.bss .bss.*)
        *(.sbss .sbss.*)
        *(COMMON)
        . = ALIGN(8);
        __bss_end = .;
    } >RAM

    . = ALIGN(8);
    PROVIDE(_end = .);

    ._user_heap_stack ORIGIN(RAM) + LENGTH(RAM) - __stack_size : {
        PROVIDE(_heap_end = .);
        . = __stack_size;
        _estack = .;
    } >RAM
}
