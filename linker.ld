OUTPUT_ARCH(riscv)
ENTRY(_start)

MEMORY
{
    RAM (rwx) : ORIGIN = 0x80000000, LENGTH = 64K
}

SECTIONS
{
    . = 0x80000000;
    
    .text : {
        *(.text._start)
        *(.text*)
    } > RAM
    
    .rodata : {
        *(.rodata*)
        *(.srodata*)
    } > RAM
    
    .data : {
        *(.data*)
        *(.sdata*)
    } > RAM
    
    .bss : {
        __bss_start = .;
        *(.bss*)
        *(.sbss*)
        *(COMMON)
        __bss_end = .;
    } > RAM
    
    . = ALIGN(4);
    _end = .;
}