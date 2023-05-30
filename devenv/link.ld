OUTPUT_ARCH( "riscv" )
ENTRY( _custom_start )

MEMORY {
    FLASH (rx)  : ORIGIN = 0x0000, LENGTH = 32K
    RAM   (xrw) : ORIGIN = 0x8000, LENGTH = 32K
}

__stack_size = 0x800;
#_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS {
    .text : {
        . = ALIGN(2);
        CMakeFiles/risc_v_test_prog.elf.dir/src/startup.s.obj(.text)
        *(.text)
        *(.text*)

        KEEP (*(.init))
        KEEP (*(.fini))

        __text_end = .;
    } >FLASH

    .rodata : {
        . = ALIGN(8);
        *(.rodata)
        *(.rodata.*)
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

    .ctors : {
        KEEP (*crtbegin.o(.ctors))
        KEEP (*crtbegin?.o(.ctors))
        KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .ctors))
        KEEP (*(SORT(.ctors.*)))
        KEEP (*(.ctors))
    } >FLASH

    .dtors : {
        KEEP (*crtbegin.o(.dtors))
        KEEP (*crtbegin?.o(.dtors))
        KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .dtors))
        KEEP (*(SORT(.dtors.*)))
        KEEP (*(.dtors))
    } >FLASH

    __data_start_flash = LOADADDR(.data);
    .data : {
        . = ALIGN(8);
        PROVIDE(__global_pointer$ = . + 0x800); # why?
        __data_start_ram = .;
        *(.data)
        *(.data.*)
        . = ALIGN(8);
        __data_end_ram = .;
        _edata = __data_end_ram;
    } >RAM AT> FLASH

    .bss : {
        . = ALIGN(8);
        __bss_start = .;
        *(.bss)
        *(.bss.*)
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
