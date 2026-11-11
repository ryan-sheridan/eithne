.section ".text.boot"

.global _start

_start:
    // stop 3 out of 4 cores (single core os)
    mrc p15, #0, r1, c0, c0, #5
    and r1, r1, #3
    cmp r1, #0
    bne halt

    // our C stack should start here and grow down
    // 0x8000 is the address the kernel is read from
    mov sp, #0x8000

    // we nede to define these later when we work with linker (c runtime stuff)
    // C global variables live here, so we need to 0 them out
    ldr r4, =__bss_start
    ldr r9, =__bss_end


    // zero out the bss section
    mov r5, #0
    mov r6, #0
    mov r7, #0
    mov r8, #0
    b       2f

1:
    // store whats in registers r5-r8 into r4 (16 bytes of 0)
    stmia r4!, {r5-r8}

2:
    // looping while r4 < r9
    cmp r4, r9
    blo 1b

    // load address of kernel_main (c function) into r3, then jump to that location
    ldr r3, =kernel_main
    blx r3

halt:
    // we jump here when kernel_main returns and we just do nothing
    wfe
    b halt

