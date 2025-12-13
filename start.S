.section .text._start
.global _start

_start:
    # Set the Stack Pointer (sp) to the end of our RAM
    # RAM starts at 0x80000000 and is size 0x10000 (64KB)
    # So the top is 0x80010000
    li sp, 0x80010000

    # Clear BSS section
    la t0, __bss_start
    la t1, __bss_end
clear_bss:
    bge t0, t1, done_bss
    sw zero, 0(t0)
    addi t0, t0, 4
    j clear_bss
done_bss:

    # Jump to the main C function
    call main

    # If main ever returns, just loop forever
hang:
    j hang