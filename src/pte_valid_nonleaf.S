// Positive control for black-parrot/black-parrot#1326.
// Non-leaf A/D/U stay clear; A/D/U retain their normal leaf meanings.
.section .text
.globl main

main:
    la t0, fail
    csrw mtvec, t0
    csrci mstatus, 8

    // Valid three-level mappings for two test-owned data pages.
    la t0, root_page_table
    la t1, middle_page_table
    srli t1, t1, 12
    slli t1, t1, 10
    ori t1, t1, 1
    sd t1, 8(t0)

    la t0, middle_page_table
    la t1, leaf_page_table
    srli t1, t1, 12
    slli t1, t1, 10
    ori t1, t1, 1
    sd t1, 0(t0)

    la t0, leaf_page_table
    la s1, supervisor_data
    srli t1, s1, 12
    slli t1, t1, 10
    ori t1, t1, 0xc7             // V/R/W/A/D, supervisor leaf
    sd t1, 0(t0)
    la s2, user_data
    srli t1, s2, 12
    slli t1, t1, 10
    ori t1, t1, 0xd7             // V/R/W/U/A/D, user leaf
    sd t1, 8(t0)

    la t0, root_page_table
    srli t0, t0, 12
    li t1, 8
    slli t1, t1, 60
    or t0, t0, t1
    csrw satp, t0
    sfence.vma

    // MPRV applies S-mode data permissions; SUM allows the user leaf.
    // Instruction fetch remains in M-mode throughout.
    csrr s0, mstatus
    li t0, (3 << 11) | (1 << 17) | (1 << 18)
    not t0, t0
    and s0, s0, t0
    li t0, (1 << 11) | (1 << 17) | (1 << 18)
    or t0, s0, t0
    csrw mstatus, t0

    li t0, 0x40000000
    li t1, 0x123
    sd t1, 0(t0)
    ld t2, 0(t0)
    bne t1, t2, fail

    li t0, 0x40001000
    li t1, 0x456
    sd t1, 0(t0)
    ld t2, 0(t0)
    bne t1, t2, fail

    // Check the physical pages as well, so an untranslated access cannot pass.
    csrw mstatus, s0
    ld t0, 0(s1)
    li t1, 0x123
    bne t0, t1, fail
    ld t0, 0(s2)
    li t1, 0x456
    bne t0, t1, fail
    csrw satp, zero
    sfence.vma

    li t0, 0x00102000
    sw zero, 0(t0)
1:
    wfi
    j 1b

.align 2
fail:
    li t0, 1 << 17
    csrc mstatus, t0
    li t0, 0x00102000
    li t1, 1
    sw t1, 0(t0)
2:
    wfi
    j 2b

.section .bss
.balign 4096
root_page_table:
    .zero 4096
middle_page_table:
    .zero 4096
leaf_page_table:
    .zero 4096
supervisor_data:
    .zero 4096
user_data:
    .zero 4096
