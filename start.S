.global _start

_start:
    # 1. Set the Stack Pointer (sp) to the end of our RAM
    # RAM starts at 0x80000000 and is size 0x10000 (64KB)
    # So the top is 0x80010000
    li sp, 0x80010000

    # 2. Jump to the main C function
    call main

    # 3. If main ever returns, just loop forever
hang:
    j hang
