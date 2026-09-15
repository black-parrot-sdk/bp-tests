// Positive control for black-parrot/black-parrot#1325.
// Supported writes must still update MODE and the root PPN.
.section .text
.globl main

main:
    la t0, fail
    csrw mtvec, t0
    csrci mstatus, 8

    csrw satp, zero
    sfence.vma
    csrr t0, satp
    bnez t0, fail

    // Remain in M-mode; the page tables are never used for translation.
    la t0, root_a
    srli t0, t0, 12
    li t1, 8
    slli t1, t1, 60
    or t0, t0, t1
    csrw satp, t0
    csrr t2, satp
    bne t2, t0, fail

    la t2, root_b
    srli t2, t2, 12
    or t2, t2, t1
    csrrw t3, satp, t2
    bne t3, t0, fail
    csrr t3, satp
    bne t3, t2, fail

    // Bare requires zero in the remaining fields.
    csrrw t3, satp, zero
    bne t3, t2, fail
    sfence.vma
    csrr t0, satp
    bnez t0, fail

    li t0, 0x00102000
    sw zero, 0(t0)
1:
    wfi
    j 1b

.align 2
fail:
    li t0, 0x00102000
    li t1, 1
    sw t1, 0(t0)
2:
    wfi
    j 2b

.section .bss
.balign 4096
root_a:
    .zero 4096
root_b:
    .zero 4096
