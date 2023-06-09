.globl _c_entry_point
.globl _c_trap_handler

.macro def name
    .globl \name;
    .align 2;
    \name:
.endm

.macro fed name
    .type \name, @function;
    .size \name, .-\name;
.endm

.section .init
def _custom_start
    la sp, _estack

    la t0, _trap_handler
    csrw mtvec, t0

    jal ra, _c_entry_point

    .loop:
        jal zero, .loop
fed _custom_start
.section .text

def _trap_handler
    call _c_trap_handler
    mret
fed _trap_handler
