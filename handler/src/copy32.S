.text

.globl _copyin
.align 4
_copyin:
    push    {r4-r7,lr}
    add     r7, sp, #0xc
    push    {r10,r11}

    mov     r3, r1
    mov     r10, #0x0
    mov     r11, #0x0

copyin_loop:
    ldrd    r4, r5, [r0], #0x8
    strd    r4, r5, [r1], #0x8
    ldrd    r4, r5, [r0], #0x8
    strd    r4, r5, [r1], #0x8
    ldrd    r4, r5, [r0], #0x8
    strd    r4, r5, [r1], #0x8
    ldrd    r4, r5, [r0], #0x8
    strd    r4, r5, [r1], #0x8
    ldrd    r4, r5, [r0], #0x8
    strd    r4, r5, [r1], #0x8
    ldrd    r4, r5, [r0], #0x8
    strd    r4, r5, [r1], #0x8

    strd    r10, r11, [r1], #0x8
    strd    r10, r11, [r1], #0x8

    subs    r2, r2, #1
    bne     copyin_loop

    sub     r0, r1, r3
    sub     r0, r0, #0x10

    pop     {r10,r11}
    pop     {r4-r7,pc}

.globl _copyout
_copyout:
    push    {r4-r7,lr}
    add     r7, sp, #0xc

copyout_loop:
    ldrd    r4, r5, [r0], #0x8
    strd    r4, r5, [r1], #0x8
    ldrd    r4, r5, [r0], #0x8
    strd    r4, r5, [r1], #0x8
    ldrd    r4, r5, [r0], #0x8
    strd    r4, r5, [r1], #0x8
    ldrd    r4, r5, [r0], #0x8
    strd    r4, r5, [r1], #0x8
    ldrd    r4, r5, [r0], #0x8
    strd    r4, r5, [r1], #0x8
    ldrd    r4, r5, [r0], #0x8
    strd    r4, r5, [r1], #0x8

    add     r0, r0, #0x10
    subs    r2, r2, #0x1
    bne     copyout_loop

    pop     {r4-r7,pc}

#if WITH_SEP
.globl _copy128
.align 4
_copy128:
    ldrd    r2, r3, [r0], #0x8
    strd    r2, r3, [r1], #0x8
    ldrd    r2, r3, [r0], #0x8
    strd    r2, r3, [r1], #0x8

    bx      lr
#endif
