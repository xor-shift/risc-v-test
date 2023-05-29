OUTPUT_ARCH( "riscv" )
ENTRY( _custom_start )

MEMORY {
    FLASH (rxai!w) : ORIGIN = 0x00000, LENGTH = 512K
    RAM   (wxa!ri) : ORIGIN = 0x80000, LENGTH = 512K
}

#_estack_upper_bits = ((ORIGIN(RAM) + LENGTH(RAM)) & 0xFFFFF000) >> 12;
#_estack_lower_bits = ((ORIGIN(RAM) + LENGTH(RAM)) & 0x00000FFF);
#PROVIDE(_estack = ORIGIN(RAM) + LENGTH(RAM));
#PROVIDE(_memory_start = ORIGIN(ram));
#PROVIDE(_memory_end = _memory_start + LENGTH(ram));

PHDRS {
  FLASH PT_LOAD;
  RAM_INIT PT_LOAD;
  RAM PT_NULL;
}

SECTIONS {
    __stack_size = DEFINED(__stack_size) ? __stack_size : 2K;

    .init : {
        KEEP(*(SORT_NONE(.init)))
    } >FLASH AT>FLASH :FLASH

	.text : {
	    CMakeFiles/risc_v_test_prog.elf.dir/src/startup.s.obj(.text)
	    #entry.o(.text)

        *(_start)
        *(.text.unlikely .text.unlikely.*)
        *(.text.startup .text.startup.*)
        *(.text .text.*)
        *(.gnu.linkonce.t.*)
	} >FLASH AT>FLASH :FLASH

    .fini : {
        KEEP(*(SORT_NONE(.fini)))
    } >FLASH AT>FLASH :FLASH

    PROVIDE(__etext = .);
    PROVIDE(_etext = .);
    PROVIDE(etext = .);

    .rodata : {
        *(.rdata)
        *(.rodata .rodata.*)
        *(.gnu.linkonce.r.*)
    } >FLASH AT>FLASH :FLASH

    . = ALIGN(4);

    .preinit_array : {
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array))
        PROVIDE_HIDDEN(__preinit_array_end = .);
    } >FLASH AT>FLASH :FLASH

    .preinit_array : {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
        KEEP(*(.init_array EXCLUDE_FILE(*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o) .ctors))
        PROVIDE_HIDDEN(__init_array_end = .);
    } >FLASH AT>FLASH :FLASH

    .fini_array : {
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT_BY_INIT_PRIORITY(.fini_array.*) SORT_BY_INIT_PRIORITY(.dtors.*)))
        KEEP(*(.fini_array EXCLUDE_FILE(*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o) .dtors))
        PROVIDE_HIDDEN(__fini_array_end = .);
    } >FLASH AT>FLASH :FLASH

    .ctors : {
        KEEP(*crtbegin.o(.ctors))
        KEEP(*crtbegin?.o(.ctors))
        KEEP(*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .ctors))
        KEEP(*(SORT(.ctors.*)))
        KEEP(*(.ctors))
    } >FLASH AT>FLASH :FLASH

    .dtors : {
        KEEP(*crtbegin.o(.dtors))
        KEEP(*crtbegin?.o(.dtors))
        KEEP(*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .dtors))
        KEEP(*(SORT(.dtors.*)))
        KEEP(*(.dtors))
    } >FLASH AT>FLASH :FLASH

    .lalign : {
        . = ALIGN(4);
        PROVIDE(_data_lma = .);
    } >FLASH AT>FLASH :FLASH

    .dalign : {
        . = ALIGN(4);
        PROVIDE(_data = .);
    } >FLASH AT>FLASH :FLASH

    .data : {
        *(.data .data.*)
        *(.gnu.linkonce.d.*)
        . = ALIGN(8);
        PROVIDE(__global_pointer$ = . + 0x800);
        *(.sdata .sdata.*)
        *(.gnu.linkonce.s.*)
        . = ALIGN(8);
        *(.srodata.cst16)
        *(.srodata.cst8)
        *(.srodata.cst4)
        *(.srodata.cst2)
        *(.srodata .srodata.*)
    } >FLASH AT>FLASH :FLASH

    . = ALIGN(4);
    PROVIDE(_edata = .);
    PROVIDE(edata = .);

    PROVIDE(_fbss = .);
    PROVIDE(__bss_start = .);
    .bss : {
        *(.sbss*)
        *(.gnu.linkonce.sb.*)
        *(.bss .bss.*)
        *(.gnu.linkonce.b.*)
        *(COMMON)
        . = ALIGN(4);
    } >FLASH AT>FLASH :FLASH
    PROVIDE(__bss_end = .);

    . = ALIGN(8);
    PROVIDE(_end = .);
    PROVIDE(end = .);

    .stack ORIGIN(RAM) + LENGTH(RAM) - __stack_size : {
        PROVIDE(_heap_end = .);
        . = __stack_size;
        PROVIDE(_estack = .);
    } >RAM AT>RAM :RAM
}
