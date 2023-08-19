.text
.align 4
.globl _copyin
_copyin:
    mov x3, x1
    movi v0.16b, #0

copyin_loop:
    ldp q1, q2, [x0], #0x20
    ldr q3, [x0], #0x10
    stp q1, q2, [x1], #0x20
    stp q3, q0, [x1], #0x20
    subs x2, x2, #1
    bne copyin_loop

    sub x0, x1, x3
    sub x0, x0, #0x10
    ret

.globl _copyout
_copyout:
    ldp q0, q1, [x0], #0x20
    ldr q2, [x0], #0x20
    stp q0, q1, [x1], #0x20
    str q2, [x1], #0x10
    subs x2, x2, #1
    bne _copyout

    ret

#if WITH_SEP
.globl _copy128
_copy128:
    ldr q0, [x0]
    str q0, [x1]
    ret
#endif

